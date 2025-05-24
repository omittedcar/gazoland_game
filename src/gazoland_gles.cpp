#include "gles_or_vulkan.h"

#include <cassert>
#include <fstream>
#include <iostream>

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
    const std::string& v_uv_name) {
  std::ifstream ifs(path.string(), std::ios::in);
  std::ostringstream oss;
  oss << ifs.rdbuf();
  std::string shader_source(oss.str());
  const char* shader_source_c = shader_source.c_str();
  GLuint id = glCreateShader(shader_type_to_gl(type));
  CHECK_GL();
  glShaderSource(id, 1, &shader_source_c, nullptr);
  CHECK_GL();
  glCompileShader(id);
  CHECK_GL();
  GLint param;
  glGetShaderiv(id, GL_COMPILE_STATUS, &param);
  CHECK_GL();
  if (param != GL_TRUE) {
    std::cerr << "glCompileShader(" << path << ") failed." << std::endl;
  }

  return std::shared_ptr<shader>(
      new shader(path.filename().stem(), id, v_pos_name, v_uv_name));
}

shader::shader(const std::string& name_arg, GLuint id_arg, const std::string& v_pos_name, const std::string& v_uv_name)
    : gl_resource(name_arg, id_arg),
      v_pos_name_(v_pos_name),
      v_uv_name_(v_uv_name) {}

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
    const std::string& u_panning_name,
    const std::string& u_projection_name,
    const std::string& u_texture_name) {
  GLuint id = glCreateProgram();
  CHECK_GL();
  if (vertex_shader) {
    glAttachShader(id, vertex_shader->id());
    CHECK_GL();
  }
  if (fragment_shader) {
    glAttachShader(id, fragment_shader->id());
    CHECK_GL();
  }
  glLinkProgram(id);
  CHECK_GL();
  GLint param = 0;
  glGetProgramiv(id, GL_LINK_STATUS, &param);
  CHECK_GL();
  if (param != GL_TRUE) {
    std::cerr << "Link failure for program " << name << std::endl;
    return nullptr;
  }

  GLint u_panning = -1, u_projection = -1, u_texture = -1;
  if (u_panning_name.length()) {
    u_panning = glGetUniformLocation(id, u_panning_name.c_str());
    CHECK_GL();
  }
  if (u_projection_name.length()) {
    u_projection = glGetUniformLocation(id, u_projection_name.c_str());
    CHECK_GL();
  }
  if (u_texture_name.length()) {
    u_texture = glGetUniformLocation(id, u_texture_name.c_str());
    CHECK_GL();
  }

  return std::shared_ptr<program>(
      new program(
          name, id, std::move(vertex_shader), std::move(fragment_shader),
          u_panning, u_projection, u_texture));
}

program::program(
    const std::string& name,
    GLuint id,
    std::shared_ptr<shader> vertex_shader,
    std::shared_ptr<shader> fragment_shader,
    GLint u_panning,
    GLint u_projection,
    GLint u_texture)
    : gl_resource(name, id),
      vertex_shader_(std::move(vertex_shader)),
      fragment_shader_(std::move(fragment_shader)),
      u_panning_(u_panning),
      u_projection_(u_projection),
      u_texture_(u_texture) {
  if (vertex_shader_) {
    if (!vertex_shader_->v_pos_name().empty()) {
      v_pos_ = glGetAttribLocation(id, vertex_shader_->v_pos_name().c_str());
      CHECK_GL();
    }
    if (!vertex_shader_->v_uv_name().empty()) {
      v_uv_ = glGetAttribLocation(id, vertex_shader_->v_uv_name().c_str());
      CHECK_GL();
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
    const std::filesystem::path& path) {
  GLuint id = 0;
  FILE* tex_file = fopen(path.c_str(), "rb");
  size_t width = 32;
  size_t height = 32;
  size_t buffer_size = width * height * 2;
  size_t mip_count = 3;
  std::vector<unsigned char> data(buffer_size);
  //note to self: the total size of the file should be 2/3 the # of pixels
  fread(data.data(), 1, width * height * 4 / 3, tex_file);
  fclose(tex_file);
  glGenTextures(1, &id);
  CHECK_GL();
  glBindTexture(GL_TEXTURE_2D, id);
  CHECK_GL();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  CHECK_GL();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  CHECK_GL();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  CHECK_GL();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  CHECK_GL();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
  CHECK_GL();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mip_count);
  CHECK_GL();
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
    CHECK_GL();
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
  CHECK_GL();
  glBindTexture(GL_TEXTURE_2D, id);
  CHECK_GL();
  glTexImage2D(
      GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, width, height, 0,
      GL_RGB,GL_UNSIGNED_INT_10F_11F_11F_REV, nullptr);
  CHECK_GL();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  CHECK_GL();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  CHECK_GL();
  glBindFramebuffer(GL_FRAMEBUFFER, fb->id());
  CHECK_GL();
  glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id, 0);
  CHECK_GL();
  return std::shared_ptr<texture>(new texture("draw_buffer", id));
}

// static
std::shared_ptr<texture> texture::create_for_depth(
    size_t width, size_t height,
    std::shared_ptr<framebuffer> fb) {
  GLuint id = 0;
  glGenTextures(1, &id);
  CHECK_GL();
  glBindTexture(GL_TEXTURE_2D, id);
  CHECK_GL();
  glTexImage2D(
      GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0,
      GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
  CHECK_GL();
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindFramebuffer(GL_FRAMEBUFFER, fb->id());
  CHECK_GL();
  glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, id, 0);
  CHECK_GL();
  return std::shared_ptr<texture>(new texture("depth_buffer", id));
}

// static
std::shared_ptr<texture> texture::create_for_gui(
    size_t width, size_t height, const std::vector<unsigned char>& lettering) {
  GLuint id = 0;
  glGenTextures(1, &id);
  CHECK_GL();
  glBindTexture(GL_TEXTURE_2D, id);
  CHECK_GL();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  CHECK_GL();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  CHECK_GL();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  CHECK_GL();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  CHECK_GL();
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width * 16, height, 0,
               GL_RED, GL_UNSIGNED_BYTE, lettering.data());
  CHECK_GL();
  return std::shared_ptr<texture>(new texture("gui_buffer", id));
}

texture::~texture() {
  if (id()) {
    GLuint id_arg = id();
    glDeleteTextures(1, &id_arg);
    CHECK_GL();
  }
}

buffer::~buffer() {
  if (id()) {
    GLuint id_arg = id();
    glDeleteBuffers(1, &id_arg);
    CHECK_GL();
  }
}

//static
std::shared_ptr<framebuffer> framebuffer::create(const std::string& name) {
  GLuint id;
  glGenFramebuffers(1, &id);
  CHECK_GL();
  return std::shared_ptr<framebuffer>(new framebuffer(name, id));
}

framebuffer::~framebuffer() {
  if (id()) {
    GLuint id_arg = id();
    glDeleteFramebuffers(1, &id_arg);
    CHECK_GL();
  }
}

void prepare_to_draw(
    const std::shared_ptr<framebuffer>& fb,
    size_t width, size_t height) {
  // bind framebuffer
  glEnable(GL_DEPTH_TEST);
  CHECK_GL();
  glDepthFunc(GL_LEQUAL);
  CHECK_GL();
  glBindFramebuffer(GL_FRAMEBUFFER, fb->id());
  CHECK_GL();
  glViewport(0, 0, width, height);
  CHECK_GL();

  // clear
  glClearColor(0.5, 0.5, 0.5, 1.0);
  CHECK_GL();
  glClearDepthf(1.0);
  CHECK_GL();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  CHECK_GL();
  glDisable(GL_BLEND);
  CHECK_GL();
}

void draw_level(
    const std::shared_ptr<program>& terrain_shader,
    const std::vector<float>& projection_matrix,
    float x, float y,
    std::shared_ptr<texture>& stone_tile_texture) {
  glEnable(GL_BLEND);
  CHECK_GL();
  glBlendFunc(GL_ONE, GL_SRC_ALPHA);
  CHECK_GL();
  glUseProgram(terrain_shader->id());
  CHECK_GL();
  glUniformMatrix4fv(
      terrain_shader->u_projection(), 1, GL_FALSE, projection_matrix.data());
  CHECK_GL();
  glUniform2f(
      terrain_shader->u_panning(), x, y);
  CHECK_GL();
  glUniform1i(terrain_shader->u_texture(), 0);
  CHECK_GL();
  glBindTexture(GL_TEXTURE_2D, stone_tile_texture->id());
  CHECK_GL();
}

void draw_platform(
    const std::shared_ptr<program>& surface_shader,
    const std::shared_ptr<program>& fill_shader,
    const std::shared_ptr<buffer>& vertex_pos_buffer,
    const std::shared_ptr<buffer>& vertex_uv_buffer,
    const std::shared_ptr<buffer>& upper_surface_index_buffer,
    const std::shared_ptr<buffer>& lower_surface_index_buffer,
    const std::shared_ptr<buffer>& corner_vertex_buffer,
    const std::shared_ptr<buffer>& inner_face_index_buffer,
    const std::vector<float>& projection,
    float x, float y, int side_count) {
  glUseProgram(surface_shader->id());
  CHECK_GL();
  glBindBuffer(GL_ARRAY_BUFFER, vertex_pos_buffer->id());
  CHECK_GL();
  glVertexAttribPointer(surface_shader->v_pos(), 3, GL_FLOAT, false, 0, nullptr);
  CHECK_GL();
  glEnableVertexAttribArray(surface_shader->v_pos());
  CHECK_GL();
  
  glBindBuffer(GL_ARRAY_BUFFER, vertex_uv_buffer->id());
  CHECK_GL();
  glVertexAttribPointer(surface_shader->v_uv(), 2, GL_FLOAT, false, 0, nullptr);
  CHECK_GL();
  glEnableVertexAttribArray(surface_shader->v_uv());
  CHECK_GL();

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, upper_surface_index_buffer->id());
  CHECK_GL();
  glDrawElements(GL_TRIANGLES, side_count * 6, GL_UNSIGNED_SHORT, nullptr);
  CHECK_GL();
  glDisableVertexAttribArray(surface_shader->v_pos());
  CHECK_GL();
  glDisableVertexAttribArray(surface_shader->v_uv());
  CHECK_GL();
  
  //glDisable(GL_BLEND);

  glUseProgram(fill_shader->id());
  CHECK_GL();
  glEnableVertexAttribArray(fill_shader->v_pos());
  CHECK_GL();
  glBindBuffer(GL_ARRAY_BUFFER, corner_vertex_buffer->id());
  CHECK_GL();
  glVertexAttribPointer(fill_shader->v_pos(), 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  CHECK_GL();
  glUniformMatrix4fv(fill_shader->u_projection(), 1, false, projection.data());
  CHECK_GL();
  glUniform1i(fill_shader->u_texture(), 0);
  CHECK_GL();
  glUniform2f(fill_shader->u_panning(), x, y);
  CHECK_GL();

  //glEnable(GL_BLEND);
  
  glBindTexture(GL_TEXTURE_2D, 6);
  CHECK_GL();
  glDisable(GL_CULL_FACE);
  CHECK_GL();
  glDisable(GL_BLEND);
  CHECK_GL();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, inner_face_index_buffer->id());
  CHECK_GL();
  glDrawElements(GL_TRIANGLES, 3*(side_count - 2), GL_UNSIGNED_SHORT, nullptr);
  CHECK_GL();

  glUseProgram(surface_shader->id());
  CHECK_GL();
  glBindBuffer(GL_ARRAY_BUFFER, vertex_pos_buffer->id());
  CHECK_GL();
  glVertexAttribPointer(surface_shader->v_pos(), 3, GL_FLOAT, false, 0, nullptr);
  CHECK_GL();
  glEnableVertexAttribArray(surface_shader->v_pos());
  CHECK_GL();
  glBindBuffer(GL_ARRAY_BUFFER, vertex_uv_buffer->id());
  CHECK_GL();
  
  glVertexAttribPointer(surface_shader->v_uv(), 2, GL_FLOAT, false, 0, nullptr);
  CHECK_GL();
  glEnableVertexAttribArray(surface_shader->v_uv());
  CHECK_GL();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lower_surface_index_buffer->id());
  CHECK_GL();
  //glEnable(GL_BLEND);
  glBindTexture(GL_TEXTURE_2D, 5);
  CHECK_GL();
  glDrawElements(GL_TRIANGLES, side_count * 6, GL_UNSIGNED_SHORT, nullptr);
  CHECK_GL();
  glDisableVertexAttribArray(surface_shader->v_pos());
  CHECK_GL();
  glDisableVertexAttribArray(surface_shader->v_uv());
  CHECK_GL();
}

void draw_gazo(
    const std::shared_ptr<program>& gazo_shader,
    float x, float y,
    const std::vector<float>& projection_matrix,
    const std::shared_ptr<texture>& gazo_spritesheet_tex,
    const std::shared_ptr<buffer> vertex_buf,
    const std::shared_ptr<buffer> uv_buf,
    const std::shared_ptr<buffer> element_index_buf,
    int uv_map_offset,
    int n_sides) {
  glUseProgram(gazo_shader->id());
  CHECK_GL();
  glUniformMatrix4fv(
      gazo_shader->u_projection(), 1, GL_FALSE, projection_matrix.data()
  );  
  CHECK_GL();
  glUniform2f(
      gazo_shader->u_panning(), x, y
  );
  CHECK_GL();
  glUniform1i(
      gazo_shader->u_texture(), 0
  );
  CHECK_GL();
  glActiveTexture(GL_TEXTURE0);
  CHECK_GL();
  glBindTexture(GL_TEXTURE_2D, gazo_spritesheet_tex->id());
  CHECK_GL();
  
  glDisable(GL_CULL_FACE);
  CHECK_GL();
  
  glUseProgram(gazo_shader->id());
  CHECK_GL();
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buf->id());
  CHECK_GL();
  if (gazo_shader->v_pos() != -1) {
    glVertexAttribPointer(gazo_shader->v_pos(), 2, GL_FLOAT, false, 0, nullptr);
    CHECK_GL();
    glEnableVertexAttribArray(gazo_shader->v_pos());
    CHECK_GL();
  }
  
  glBindBuffer(GL_ARRAY_BUFFER, uv_buf->id());
  CHECK_GL();
  if (gazo_shader->v_uv() != -1) {
    glVertexAttribPointer(
        gazo_shader->v_uv(), 2, GL_FLOAT, false, 0,
        (void*) (long long int) (uv_map_offset * 0x80));
    CHECK_GL();
    glEnableVertexAttribArray(gazo_shader->v_uv());
    CHECK_GL();
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_index_buf->id());
  CHECK_GL();
  glLineWidth(2);
  CHECK_GL();

  glDrawElements(GL_TRIANGLES, n_sides * 3, GL_UNSIGNED_SHORT, nullptr);
  CHECK_GL();
  glEnable(GL_BLEND);
  CHECK_GL();
  glBlendColor(0.0, 0.0, 0.0, 0.5);
  CHECK_GL();
  glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
  CHECK_GL();
  glBindTexture(GL_TEXTURE_2D, 0);
  CHECK_GL();

  glDrawArrays(GL_LINE_LOOP, 1, n_sides);
  CHECK_GL();
  glDisable(GL_BLEND);
  CHECK_GL();
  glLineWidth(1);
  CHECK_GL();
  glDrawArrays(GL_LINE_LOOP, 1, n_sides);
  CHECK_GL();

  if (gazo_shader->v_uv() != -1) {
    glDisableVertexAttribArray(gazo_shader->v_uv());
    CHECK_GL();
  }
  if (gazo_shader->v_pos() != -1) {
    glDisableVertexAttribArray(gazo_shader->v_pos());
    CHECK_GL();
  }
}

void present_game(
    size_t width, size_t height,
    const std::shared_ptr<program>& gamma_prog,
    const std::shared_ptr<buffer>& square_buf,
    const std::shared_ptr<texture>& draw_tex) {
  glViewport(0, 0, (GLsizei) width, (GLsizei) height);
  CHECK_GL();
  glDisable(GL_BLEND);
  CHECK_GL();
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  CHECK_GL();
  glClearDepthf(1.0f);
  CHECK_GL();
  glClear(GL_DEPTH_BUFFER_BIT);
  CHECK_GL();
  glUseProgram(gamma_prog->id());
  CHECK_GL();
  glBindBuffer(GL_ARRAY_BUFFER, square_buf->id());
  CHECK_GL();
  glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, nullptr);
  CHECK_GL();
  glEnableVertexAttribArray(0);
  CHECK_GL();
  glBindTexture(GL_TEXTURE_2D, draw_tex->id());
  CHECK_GL();
  glDrawArrays(GL_TRIANGLES, 0, 6);
  CHECK_GL();
}

void present_gui(
    const std::shared_ptr<program>& gui_prog,
    const std::shared_ptr<buffer>& square_buf,
    const std::shared_ptr<texture>& gui_tex) {
  glUseProgram(gui_prog->id());
  CHECK_GL();
  glBindBuffer(GL_ARRAY_BUFFER, square_buf->id());
  CHECK_GL();
  glVertexAttribPointer(gui_prog->v_pos(), 2, GL_FLOAT, false, 0, nullptr);
  CHECK_GL();
  glEnableVertexAttribArray(gui_prog->v_pos());
  CHECK_GL();
  glUniform1i(gui_prog->u_texture(), 0);
  CHECK_GL();
  glBindTexture(GL_TEXTURE_2D, gui_tex->id());
  CHECK_GL();
  glDrawArrays(GL_TRIANGLES, 0, 6);
  CHECK_GL();
}
