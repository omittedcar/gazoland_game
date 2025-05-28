#ifndef _GAZOLAND_SRC_GAZOLAND_GLES_H_
#define _GAZOLAND_SRC_GAZOLAND_GLES_H_

#include "vec2.h"

#include <GLES3/gl3.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

void gazoland_init();
void gazoland_cleanup();

GLenum shader_type_to_gl(shader_type type);
GLenum texture_type_to_gl(texture_type type);
GLenum buffer_type_to_gl(buffer_type type);

class gl_resource {
 public:
  gl_resource() = delete;
  gl_resource(const gl_resource&) = delete;
  virtual ~gl_resource();

  const std::string& name() const { return name_; }
  const GLuint id() const { return id_; }

 protected:
  gl_resource(const std::string& name_arg, GLuint id_arg)
    : name_(name_arg), id_(id_arg) {}

 private:
  std::string name_;
  GLuint id_;
};

class framebuffer : public gl_resource {
public:
  ~framebuffer() override;
  framebuffer() = delete;
  framebuffer(const framebuffer&) = delete;

  static std::shared_ptr<framebuffer> create(const std::string& name);

 private:
  framebuffer(const std::string& name, GLuint id)
      : gl_resource(name, id) {}
};

class buffer : public gl_resource {
public:
  ~buffer() override;
  buffer() = delete;
  buffer(const buffer&) = delete;

  template<class T>
  static std::shared_ptr<buffer> create(const std::string& name,
                                        const std::vector<T>& data,
					buffer_type type) {
    GLuint id;
    glGenBuffers(1, &id);
    if (data.size()) {
      glBindBuffer(buffer_type_to_gl(type), id);
      glBufferData(buffer_type_to_gl(type),
		   data.size() * sizeof(T),
		   data.data(), GL_STATIC_DRAW);
    }
    return std::shared_ptr<buffer>(new buffer(name, id));
  }

  template<class T>
  void update(const std::vector<T>& data, buffer_type type) {
    glBindBuffer(buffer_type_to_gl(type), id());
    glBufferData(buffer_type_to_gl(type),
                 data.size() * sizeof(T),
                 data.data(), GL_STATIC_DRAW);
  }

 private:
  buffer(const std::string& name, GLuint id)
      : gl_resource(name, id) {}
};

class shader : public gl_resource {
public:
  ~shader() override;
  shader() = delete;
  shader(const shader&) = delete;

  static std::shared_ptr<shader> create(
      const std::filesystem::path& path,
      shader_type shader_type_arg,
      const std::string& v_pos_name,
      const std::string& v_uv_name);

  const std::string& v_pos_name() const { return v_pos_name_; }
  const std::string& v_uv_name() const { return v_uv_name_; }

private:
  shader(
      const std::string& name_arg, GLuint id,
      const std::string& v_pos_name, const std::string& v_uv_name);

  std::string v_pos_name_;
  std::string v_uv_name_;
};

class program : public gl_resource {
public:
  ~program() override;
  program() = delete;
  program(const program&) = delete;

  static std::shared_ptr<program> create(
      const std::string& name,
      std::shared_ptr<shader> vertex_shader,
      std::shared_ptr<shader> fragment_shader,
      const std::string& u_panning_name,
      const std::string& u_projection_name,
      const std::string& u_texture_name);

  GLint v_pos() const { return v_pos_; }
  GLint v_uv() const { return v_uv_; }
  GLint u_panning() const { return u_panning_; }
  GLint u_projection() const { return u_projection_; }
  GLint u_texture() const { return u_texture_; }

private:
  program(const std::string& name,
	  GLuint id,
	  std::shared_ptr<shader> vertex_shader,
	  std::shared_ptr<shader> fragment_shader,
          GLint u_panning, GLint u_projection, GLint u_texture);

  std::shared_ptr<shader> vertex_shader_;
  std::shared_ptr<shader> fragment_shader_;
  GLint v_pos_{-1};
  GLint v_uv_{-1};
  GLint u_panning_{-1};
  GLint u_projection_{-1};
  GLint u_texture_{-1};
};

class texture : public gl_resource {
public:
  ~texture() override;
  texture() = delete;
  texture(const texture&) = delete;

  static std::shared_ptr<texture> create_from_file(
      const std::filesystem::path& path,
      size_t mip_count);
  static std::shared_ptr<texture> create_for_draw(
      size_t width, size_t height,
      std::shared_ptr<framebuffer> draw_framebuffer);
  static std::shared_ptr<texture> create_for_depth(
      size_t width, size_t height,
      std::shared_ptr<framebuffer> fb);
  static std::shared_ptr<texture> create_for_gui(
      size_t width, size_t height, const std::vector<unsigned char>& lettering);
  
private:
  texture(const std::string& name, GLuint id)
      : gl_resource(name, id) {}
  };

void prepare_to_draw(
    const std::shared_ptr<framebuffer>& fb,
    size_t width, size_t height);

void draw_level(
    const std::shared_ptr<program>& terrain_shader,
    const std::vector<float>& projection_matrix,
    float x, float y,
    std::shared_ptr<texture>& stone_tile_texture);

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
    float x, float y, int side_count);

void draw_gazo(
    const std::shared_ptr<program>& gazo_shader,
    float x, float y,
    const std::vector<float>& projection_matrix,
    const std::shared_ptr<texture>& gazo_spritesheet_tex,
    const std::shared_ptr<buffer> vertex_buf,
    const std::shared_ptr<buffer> uv_buf,
    const std::shared_ptr<buffer> element_index_buf,
    int uv_map_offset,
    int n_sides);

void present_game(
    size_t width, size_t height,
    const std::shared_ptr<program>& gamma_prog,
    const std::shared_ptr<buffer>& square_buf,
    const std::shared_ptr<texture>& draw_tex);

void present_gui(
    const std::shared_ptr<program>& gui_prog,
    const std::shared_ptr<buffer>& square_buf,
    const std::shared_ptr<texture>& gui_tex);

#endif  // #ifndef _GAZOLAND_SRC_GAZOLAND_GLES_H_
