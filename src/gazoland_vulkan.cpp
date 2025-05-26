#include "gles_or_vulkan.h"

vk_resource::~vk_resource() {}

// static
std::shared_ptr<framebuffer> framebuffer::create(const std::string& name) {
  return std::shared_ptr<framebuffer>(new framebuffer(name));
}

framebuffer::framebuffer(const std::string& name_arg)
    : vk_resource(name_arg) {}

framebuffer::~framebuffer() {}

buffer::buffer(const std::string& name_arg)
    : vk_resource(name_arg) {}

buffer::~buffer() {}
  
// static
std::shared_ptr<shader> shader::create(
    const std::filesystem::path& path,
    shader_type shader_type_arg,
    const std::string& v_pos_name,
    const std::string& v_uv_name) {
  return std::shared_ptr<shader>(new shader(path, v_pos_name, v_uv_name));
}

shader::shader(
    const std::string& name_arg,
    const std::string& v_pos_name,
    const std::string& v_uv_name)
    : vk_resource(name_arg),
      v_pos_name_(v_pos_name),
      v_uv_name_(v_uv_name) {}

shader::~shader() {}

// static
std::shared_ptr<program> program::create(
    const std::string& name,
    std::shared_ptr<shader> vertex_shader,
    std::shared_ptr<shader> fragment_shader,
    const std::string& u_panning_name,
    const std::string& u_projection_name,
    const std::string& u_texture_name) {
  return std::shared_ptr<program>(
      new program(name, vertex_shader, fragment_shader));
}

program::program(
    const std::string& name,
    std::shared_ptr<shader> vertex_shader,
    std::shared_ptr<shader> fragment_shader)
    : vk_resource(name),
      vertex_shader_(vertex_shader),
      fragment_shader_(fragment_shader) {}

program::~program() {}

// static
std::shared_ptr<texture> texture::create_from_file(
    const std::filesystem::path& path) {
  return std::shared_ptr<texture>(new texture(path));
}

// static
std::shared_ptr<texture> texture::create_for_draw(
    size_t width, size_t height,
    std::shared_ptr<framebuffer> draw_framebuffer) {
  return std::shared_ptr<texture>(new texture("draw_buf"));
}

// static
std::shared_ptr<texture> texture::create_for_depth(
    size_t width, size_t height,
    std::shared_ptr<framebuffer> fb) {
  return std::shared_ptr<texture>(new texture("depth_buf"));
}
  
// static
std::shared_ptr<texture> texture::create_for_gui(
    size_t width, size_t height,
    const std::vector<unsigned char>& lettering) {
  return std::shared_ptr<texture>(new texture("gui"));
}

texture::texture(const std::string& name)
    : vk_resource(name) {}

texture::~texture() {}

void prepare_to_draw(
    const std::shared_ptr<framebuffer>& fb,
    size_t width, size_t height) {}

void draw_level(
    const std::shared_ptr<program>& terrain_shader,
    const std::vector<float>& projection_matrix,
    float x, float y,
    std::shared_ptr<texture>& stone_tile_texture) {}

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
    float x, float y, int side_count) {}

void draw_gazo(
    const std::shared_ptr<program>& gazo_shader,
    float x, float y,
    const std::vector<float>& projection_matrix,
    const std::shared_ptr<texture>& gazo_spritesheet_tex,
    const std::shared_ptr<buffer> vertex_buf,
    const std::shared_ptr<buffer> uv_buf,
    const std::shared_ptr<buffer> element_index_buf,
    int uv_map_offset,
    int n_sides) {}

void present_game(
    size_t width, size_t height,
    const std::shared_ptr<program>& gamma_prog,
    const std::shared_ptr<buffer>& square_buf,
    const std::shared_ptr<texture>& draw_tex) {}

void present_gui(
    const std::shared_ptr<program>& gui_prog,
    const std::shared_ptr<buffer>& square_buf,
    const std::shared_ptr<texture>& gui_tex) {}
