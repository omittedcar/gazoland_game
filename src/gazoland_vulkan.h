#ifndef _GAZOLAND_SRC_GAZOLAND_VULKAN_H_
#define _GAZOLAND_SRC_GAZOLAND_VULKAN_H_

#include "vec2.h"

#include <vulkan/vulkan.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

void gazoland_init();
void gazoland_cleanup();

class gazo;
class level;
class platform;

class vk_resource {
 public:
  vk_resource() = delete;
  vk_resource(const vk_resource&) = delete;
  virtual ~vk_resource();

  const std::string& name() const { return name_; }

 protected:
  vk_resource(const std::string& name_arg)
    : name_(name_arg) {}

 private:
  std::string name_;
};

class framebuffer : public vk_resource {
public:
  ~framebuffer() override;
  framebuffer() = delete;
  framebuffer(const framebuffer&) = delete;

  static std::shared_ptr<framebuffer> create(const std::string& name);

 private:
  framebuffer(const std::string& name);
};

class buffer : public vk_resource {
public:
  ~buffer() override;
  buffer() = delete;
  buffer(const buffer&) = delete;

  template<class T>
  static std::shared_ptr<buffer> create(const std::string& name,
                                        const std::vector<T>& data,
					buffer_type type) {
    return std::shared_ptr<buffer>(new buffer(name));
  }

  template<class T>
  void update(const std::vector<T>& data, buffer_type type) {
    // TODO
  }

 private:
  buffer(const std::string& name);
};

class shader : public vk_resource {
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
      const std::string& name_arg,
      const std::string& v_pos_name,
      const std::string& v_uv_name);

  std::string v_pos_name_;
  std::string v_uv_name_;
};

class program : public vk_resource {
public:
  ~program() override;
  program() = delete;
  program(const program&) = delete;

  static std::shared_ptr<program> create(
      const std::string& name,
      std::shared_ptr<shader> vertex_shader,
      std::shared_ptr<shader> fragment_shader,
      const std::string& u_projection_name,
      const std::string& u_texture_name);    

private:
  program(const std::string& name,
	  std::shared_ptr<shader> vertex_shader,
	  std::shared_ptr<shader> fragment_shader);

  std::shared_ptr<shader> vertex_shader_;
  std::shared_ptr<shader> fragment_shader_;
};

class texture : public vk_resource {
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
      size_t width, size_t height, const unsigned char* lettering);
  
private:
  texture(const std::string& name);
};

void prepare_to_draw(
    const std::shared_ptr<framebuffer>& fb,
    size_t width, size_t height);

void draw_platform(
    const platform& pl,
    const std::vector<float>& projection,
    fvec2 view);

void draw_gazo(
    const gazo& gz,
    const std::vector<float>& projection_matrix,
    fvec2 view);

void present_game(
    size_t width, size_t height,
    const std::shared_ptr<program>& gamma_prog,
    const std::shared_ptr<buffer>& square_buf,
    const std::shared_ptr<texture>& draw_tex);

void present_gui(
    const std::shared_ptr<program>& gui_prog,
    const std::shared_ptr<buffer>& square_buf,
    const std::shared_ptr<texture>& gui_tex);

#endif  // #ifndef _GAZOLAND_SRC_GAZOLAND_VULKAN_H_
