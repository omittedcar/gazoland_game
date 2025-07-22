#include "gles_or_vulkan.h"
#include "gazo.h"
#include "level.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <optional>

#ifndef NDEBUG
# define CHECK_GL() maybe_print_gl_error(__FILE__, __LINE__)
#else
# define CHECK_GL() __asm(nop)
#endif

namespace {

bool maybe_print_gl_error(const char *file, size_t line) {
  GLenum err;
  bool result = false;
  while ((err = glGetError()) != GL_NO_ERROR) {
    result = true;
    std::cerr << "GL error code 0x" << std::hex << err << std::dec
              << " at " << file << ":" << line << std::endl;
  }
  return result;
}

constexpr int s_projection_binding = 0;
GLint projection_view_offset = 0;
GLint projection_matrix_offset = 0;
std::shared_ptr<buffer> projection_buffer;

std::ostream& operator<<(std::ostream& os, const fvec2& v) {
  os << std::right << std::setprecision(2) << std::fixed;
  os << "(" << v.x << ", " << v.y << ")";
  os << std::left << std::setprecision(0) << std::defaultfloat;
  return os;
}

std::ostream& dump_matrix(std::ostream& os,
                          const std::vector<float>& matrix) {
  os << "[ " << std::right << std::setprecision(2) << std::fixed;
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      os << std::setw(4) << matrix[(row*4) + col];
      if (col < 3) os << ", ";
    }
    if (row < 3) os << std::endl << "  ";
  }
  os << std::setw(0) << std::left << std::defaultfloat << " ]" << std::endl;
  return os;
}

}  // namespace

void gazoland_init() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwSwapInterval(1);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
}

void gazoland_cleanup() {}

GLenum shader_type_to_gl(shader_type type) {
  switch (type) {
  case shader_type::k_vertex:
    return GL_VERTEX_SHADER;
  case shader_type::k_fragment:
    return GL_FRAGMENT_SHADER;
  default:
    assert(false);
  }
  return 0;
}

GLenum texture_type_to_gl(texture_type type) {
  switch (type) {
  case texture_type::k_2d:
    return GL_TEXTURE_2D;
  default:
    assert(false);
  }
  return 0;
}

GLenum buffer_type_to_gl(buffer_type type) {
  switch (type) {
  case buffer_type::k_array:
    return GL_ARRAY_BUFFER;
  case buffer_type::k_uniform:
    return GL_UNIFORM_BUFFER;
  default:
    assert(false);
  }
  return 0;
}

gl_resource::~gl_resource() {}

// static
std::shared_ptr<shader> shader::create(
    const std::filesystem::path& path,
    shader_type type,
    const std::string& v_pos_name,
    const std::string& v_uv_name,
    bool uses_projection) {
  std::ifstream ifs(path.string(), std::ios::in);
  std::ostringstream oss;
  oss << ifs.rdbuf();
  std::string shader_source(oss.str());
  const char* shader_source_c = shader_source.c_str();
  GLuint id = glCreateShader(shader_type_to_gl(type));
   
  glShaderSource(id, 1, &shader_source_c, nullptr);
   
  glCompileShader(id);
   
  GLint param;
  glGetShaderiv(id, GL_COMPILE_STATUS, &param);
   
  if (param != GL_TRUE) {
    GLint maxLength = 0;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &maxLength);
    std::vector<GLchar> infoLog(maxLength);
    glGetShaderInfoLog(id, maxLength, &maxLength, &infoLog[0]);
    std::cerr << "glCompileShader(" << path << ") failed." << std::endl
              << infoLog.data() << std::endl;
    return nullptr;
  }

  return std::shared_ptr<shader>(
      new shader(path.filename().stem(), id, v_pos_name, v_uv_name, uses_projection));
}

shader::shader(const std::string& name_arg, GLuint id_arg, const std::string& v_pos_name, const std::string& v_uv_name, bool uses_projection)
    : gl_resource(name_arg, id_arg),
      v_pos_name_(v_pos_name),
      v_uv_name_(v_uv_name),
      uses_projection_(uses_projection) {}

shader::~shader() {
  if (id()) {
    glDeleteShader(id());
  }
}

// static
std::shared_ptr<program> program::create(
    const std::string& name,
    std::shared_ptr<shader> vertex_shader,
    std::shared_ptr<shader> fragment_shader,
    const std::string& u_texture_name) {
  GLuint id = glCreateProgram();
   
  if (vertex_shader) {
    glAttachShader(id, vertex_shader->id());
     
  }
  if (fragment_shader) {
    glAttachShader(id, fragment_shader->id());
     
  }
  glLinkProgram(id);
   
  GLint param = 0;
  glGetProgramiv(id, GL_LINK_STATUS, &param);
   
  if (param != GL_TRUE) {
    GLint maxLength = 0;
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &maxLength);
    std::vector<GLchar> infoLog(maxLength);
    glGetProgramInfoLog(id, maxLength, &maxLength, &infoLog[0]);
    std::cerr << "Link failure for program " << name << ":" << std::endl
              << infoLog.data() << std::endl;
    return nullptr;
  }

  if (vertex_shader->uses_projection()) {
    GLuint projection_block_index = glGetUniformBlockIndex(
        id, "Projection");
    glUniformBlockBinding(id, projection_block_index, s_projection_binding);

    if (!projection_buffer) {
      const char* uniform_names[2]{"Projection.view", "Projection.matrix"};
      GLuint uniform_indices[2];
      glGetUniformIndices(id, 2u, uniform_names, uniform_indices);
      GLint uniform_offsets[2];
      glGetActiveUniformsiv(
          id, 2u, uniform_indices, GL_UNIFORM_OFFSET, uniform_offsets);
      projection_view_offset = uniform_offsets[0];
      projection_matrix_offset = uniform_offsets[1];

      projection_buffer = buffer::create(
          "projection",
          std::vector<float>((uniform_offsets[1]/sizeof(float)) + 16, 0),
          buffer_type::k_uniform);
      glBindBufferBase(GL_UNIFORM_BUFFER, s_projection_binding,
                       projection_buffer->id());
    }
  }

  GLint u_texture = -1;
  if (u_texture_name.length()) {
    u_texture = glGetUniformLocation(id, u_texture_name.c_str());
  }

  return std::shared_ptr<program>(
      new program(
          name, id, std::move(vertex_shader), std::move(fragment_shader),
          u_texture));
}

program::program(
    const std::string& name,
    GLuint id,
    std::shared_ptr<shader> vertex_shader,
    std::shared_ptr<shader> fragment_shader,
    GLint u_texture)
    : gl_resource(name, id),
      vertex_shader_(std::move(vertex_shader)),
      fragment_shader_(std::move(fragment_shader)),
      u_texture_(u_texture) {
  if (vertex_shader_) {
    if (!vertex_shader_->v_pos_name().empty()) {
      v_pos_ = glGetAttribLocation(id, vertex_shader_->v_pos_name().c_str());
       
    }
    if (!vertex_shader_->v_uv_name().empty()) {
      v_uv_ = glGetAttribLocation(id, vertex_shader_->v_uv_name().c_str());
       
    }
  }
}

program::~program() {
  if (id()) {
    glDeleteProgram(id());
  }
}

// static
std::shared_ptr<texture> texture::create_from_file(
    const std::filesystem::path& path,
    size_t mip_count) {
  GLuint id = 0;
  FILE* tex_file = fopen(path.c_str(), "rb");
  size_t width = 32;
  size_t height = 32;
  size_t buffer_size = width * height;
  std::vector<unsigned char> data(buffer_size);
  //note to self: the total size of the file should be 2/3 the # of pixels
  fread(data.data(), 1, width * height, tex_file);
  fclose(tex_file);
  glGenTextures(1, &id);
   
  glBindTexture(GL_TEXTURE_2D, id);
   
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
   
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mip_count);
   
  void* mip_pointer = data.data();
  for( int mip_level = 0; mip_level <= mip_count; mip_level++){
    int mip_size = width * height >> (mip_level * 2);
    glCompressedTexImage2D(
        GL_TEXTURE_2D,
        mip_level,
        GL_COMPRESSED_RGBA8_ETC2_EAC,
        width >> mip_level,
        height >> mip_level,
        0,
        mip_size,
        mip_pointer);
     
    mip_pointer = (void*)((char*) mip_pointer + mip_size);
  }
  return std::shared_ptr<texture>(
      new texture(path.filename(), id));
}

// static
std::shared_ptr<texture> texture::create_for_draw(
    size_t width, size_t height, std::shared_ptr<framebuffer> fb) {
  GLuint id = 0;
  glGenTextures(1, &id);
   
  glBindTexture(GL_TEXTURE_2D, id);
   
  glTexImage2D(
      GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, width, height, 0,
      GL_RGB,GL_UNSIGNED_INT_10F_11F_11F_REV, nullptr);
   
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   
  glBindFramebuffer(GL_FRAMEBUFFER, fb->id());
   
  glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id, 0);
   
  return std::shared_ptr<texture>(new texture("draw_buffer", id));
}

// static
std::shared_ptr<texture> texture::create_for_depth(
    size_t width, size_t height,
    std::shared_ptr<framebuffer> fb) {
  GLuint id = 0;
  glGenTextures(1, &id);
   
  glBindTexture(GL_TEXTURE_2D, id);
   
  glTexImage2D(
      GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0,
      GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
   
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindFramebuffer(GL_FRAMEBUFFER, fb->id());
   
  glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, id, 0);
   
  return std::shared_ptr<texture>(new texture("depth_buffer", id));
}

// static
std::shared_ptr<texture> texture::create_for_gui(
    size_t width, size_t height, const unsigned char* lettering) {
  GLuint id = 0;
  glGenTextures(1, &id);
   
  glBindTexture(GL_TEXTURE_2D, id);
   
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width * 16, height, 0,
               GL_RED, GL_UNSIGNED_BYTE, lettering);
   
  return std::shared_ptr<texture>(new texture("gui_buffer", id));
}

texture::~texture() {
  if (id()) {
    GLuint id_arg = id();
    glDeleteTextures(1, &id_arg);
     
  }
}

buffer::~buffer() {
  if (id()) {
    GLuint id_arg = id();
    glDeleteBuffers(1, &id_arg);
  }
}

//static
std::shared_ptr<framebuffer> framebuffer::create(const std::string& name) {
  GLuint id;
  glGenFramebuffers(1, &id);
   
  return std::shared_ptr<framebuffer>(new framebuffer(name, id));
}

framebuffer::~framebuffer() {
  if (id()) {
    GLuint id_arg = id();
    glDeleteFramebuffers(1, &id_arg);
     
  }
}

void prepare_to_draw(
    const std::shared_ptr<framebuffer>& fb,
    size_t width, size_t height,
    const std::vector<float>& projection_matrix,
    const fvec2& view) {
  glBindBuffer(GL_UNIFORM_BUFFER, projection_buffer->id());
  glBufferSubData(
      GL_UNIFORM_BUFFER, projection_view_offset,
      2*sizeof(float), &view);
  glBufferSubData(
      GL_UNIFORM_BUFFER, projection_matrix_offset,
      16*sizeof(float), projection_matrix.data());
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  // bind framebuffer
  glEnable(GL_DEPTH_TEST);
   
  glDepthFunc(GL_LEQUAL);
   
  glBindFramebuffer(GL_FRAMEBUFFER, fb->id());
   
  glViewport(0, 0, width, height);
   

  // clear
  glClearColor(0.5, 0.5, 0.5, 1.0);
   
  glClearDepthf(1.0);
   
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   
  glDisable(GL_BLEND);
   
}

void draw_platform(
    const platform& pl,
    const std::vector<float>& projection_matrix,
    fvec2 view) {
  const std::shared_ptr<program>& terrain_shader
      = pl.get_terrain_prog();
  const std::shared_ptr<program>& fill_shader
      = pl.get_polygon_fill_prog();
  const std::shared_ptr<texture>& fill_texture
      = pl.get_stone_tile_tex();
  const std::shared_ptr<buffer>& vertex_pos_buffer
      = pl.get_vertex_pos_buffer();
  const std::shared_ptr<buffer>& vertex_uv_buffer
      = pl.get_vertex_uv_buffer();
  const std::shared_ptr<buffer>& upper_surface_index_buffer
      = pl.get_upper_surface_index_buffer();
  const std::shared_ptr<buffer>& lower_surface_index_buffer
      = pl.get_lower_surface_index_buffer();
  const std::shared_ptr<buffer>& corner_vertex_buffer
      = pl.get_corner_vertex_buffer();
  const std::shared_ptr<buffer>& inner_face_index_buffer
      = pl.get_inner_face_index_buffer();

  glEnable(GL_BLEND);
   
  glBlendFunc(GL_ONE, GL_SRC_ALPHA);
   
  glUseProgram(terrain_shader->id());

  glUniform1i(terrain_shader->u_texture(), 0);

  glBindTexture(GL_TEXTURE_2D, fill_texture->id());

  glBindBuffer(GL_ARRAY_BUFFER, vertex_pos_buffer->id());

  glVertexAttribPointer(terrain_shader->v_pos(), 3, GL_FLOAT, false, 0, nullptr);
   
  glEnableVertexAttribArray(terrain_shader->v_pos());
  
  glBindBuffer(GL_ARRAY_BUFFER, vertex_uv_buffer->id());
   
  glVertexAttribPointer(terrain_shader->v_uv(), 2, GL_FLOAT, false, 0, nullptr);
   
  glEnableVertexAttribArray(terrain_shader->v_uv());

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, upper_surface_index_buffer->id());
   
  glDrawElements(GL_TRIANGLES, pl.get_side_count() * 6, GL_UNSIGNED_SHORT, nullptr);

  glDisableVertexAttribArray(terrain_shader->v_pos());
   
  glDisableVertexAttribArray(terrain_shader->v_uv());
  
  //glDisable(GL_BLEND);

  glUseProgram(fill_shader->id());
   
  glEnableVertexAttribArray(fill_shader->v_pos());
   
  glBindBuffer(GL_ARRAY_BUFFER, corner_vertex_buffer->id());
   
  glVertexAttribPointer(fill_shader->v_pos(), 2, GL_FLOAT, GL_FALSE, 0, nullptr);
   
  glUniform1i(fill_shader->u_texture(), 0);

  //glEnable(GL_BLEND);
  
  glBindTexture(GL_TEXTURE_2D, 6);
   
  glDisable(GL_CULL_FACE);
   
  //glDisable(GL_BLEND);
   
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, inner_face_index_buffer->id());
   
  glDrawElements(GL_TRIANGLES, 3*(pl.get_side_count() - 2), GL_UNSIGNED_SHORT, nullptr);
   

  glUseProgram(terrain_shader->id());
   
  glBindBuffer(GL_ARRAY_BUFFER, vertex_pos_buffer->id());
   
  glVertexAttribPointer(terrain_shader->v_pos(), 3, GL_FLOAT, false, 0, nullptr);
   
  glEnableVertexAttribArray(terrain_shader->v_pos());
   
  glBindBuffer(GL_ARRAY_BUFFER, vertex_uv_buffer->id());
   
  
  glVertexAttribPointer(terrain_shader->v_uv(), 2, GL_FLOAT, false, 0, nullptr);
   
  glEnableVertexAttribArray(terrain_shader->v_uv());
   
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lower_surface_index_buffer->id());
   
  //glEnable(GL_BLEND);
  glBindTexture(GL_TEXTURE_2D, 5);
   
  glDrawElements(GL_TRIANGLES, pl.get_side_count() * 6, GL_UNSIGNED_SHORT, nullptr);
   
  glDisableVertexAttribArray(terrain_shader->v_pos());
   
  glDisableVertexAttribArray(terrain_shader->v_uv());
   
}

void draw_gazo(const gazo& gz,
               const std::vector<float>& projection_matrix,
               fvec2 view) {
  const std::shared_ptr<program>& gazo_shader = gz.get_prog();
  const std::shared_ptr<texture>& gazo_spritesheet_tex = gz.get_tex();
  const std::shared_ptr<buffer> vertex_buf = gz.get_vertex_buf();
  const std::shared_ptr<buffer> uv_buf = gz.get_uv_buf();
  const std::shared_ptr<buffer> element_index_buf = gz.get_element_index_buf();
  int uv_map_offset = gz.get_uv_map_offset();
  int n_sides = gz.get_n_sides();

  glUseProgram(gazo_shader->id());
   
  glUniform1i(
      gazo_shader->u_texture(), 0
  );
   
  glActiveTexture(GL_TEXTURE0);
   
  glBindTexture(GL_TEXTURE_2D, gazo_spritesheet_tex->id());
   
  
  glDisable(GL_CULL_FACE);
   
  
  glUseProgram(gazo_shader->id());
   
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buf->id());
   
  if (gazo_shader->v_pos() != -1) {
    glVertexAttribPointer(gazo_shader->v_pos(), 2, GL_FLOAT, false, 0, nullptr);
     
    glEnableVertexAttribArray(gazo_shader->v_pos());
     
  }
  
  glBindBuffer(GL_ARRAY_BUFFER, uv_buf->id());
   
  if (gazo_shader->v_uv() != -1) {
    glVertexAttribPointer(
        gazo_shader->v_uv(), 2, GL_FLOAT, false, 0,
        (void*) (long long int) (uv_map_offset * 0x80));
     
    glEnableVertexAttribArray(gazo_shader->v_uv());
     
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_index_buf->id());
   
  glLineWidth(2);
   

  glDrawElements(GL_TRIANGLES, n_sides * 3, GL_UNSIGNED_SHORT, nullptr);
   
  glEnable(GL_BLEND);
   
  glBlendColor(0.0, 0.0, 0.0, 0.5);
   
  glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
   
  glBindTexture(GL_TEXTURE_2D, 0);
   

  glDrawArrays(GL_LINE_LOOP, 1, n_sides);
   
  glDisable(GL_BLEND);
   
  glLineWidth(1);
   
  glDrawArrays(GL_LINE_LOOP, 1, n_sides);
   

  if (gazo_shader->v_uv() != -1) {
    glDisableVertexAttribArray(gazo_shader->v_uv());
     
  }
  if (gazo_shader->v_pos() != -1) {
    glDisableVertexAttribArray(gazo_shader->v_pos());
     
  }
}

void present_game(
    size_t width, size_t height,
    const std::shared_ptr<program>& gamma_prog,
    const std::shared_ptr<buffer>& square_buf,
    const std::shared_ptr<texture>& draw_tex) {
  glViewport(0, 0, (GLsizei) width, (GLsizei) height);
   
  glDisable(GL_BLEND);
   
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
   
  glClearDepthf(1.0f);
   
  glClear(GL_DEPTH_BUFFER_BIT);
   
  glUseProgram(gamma_prog->id());
   
  glBindBuffer(GL_ARRAY_BUFFER, square_buf->id());
   
  glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, nullptr);
   
  glEnableVertexAttribArray(0);
   
  glBindTexture(GL_TEXTURE_2D, draw_tex->id());
   
  glDrawArrays(GL_TRIANGLES, 0, 6);
   
}

void present_gui(
    const std::shared_ptr<program>& gui_prog,
    const std::shared_ptr<buffer>& square_buf,
    const std::shared_ptr<texture>& gui_tex) {
  glUseProgram(gui_prog->id());
   
  glBindBuffer(GL_ARRAY_BUFFER, square_buf->id());
   
  glVertexAttribPointer(gui_prog->v_pos(), 2, GL_FLOAT, false, 0, nullptr);
   
  glEnableVertexAttribArray(gui_prog->v_pos());
   
  glUniform1i(gui_prog->u_texture(), 0);
   
  glBindTexture(GL_TEXTURE_2D, gui_tex->id());
   
  glDrawArrays(GL_TRIANGLES, 0, 6);
   
}
