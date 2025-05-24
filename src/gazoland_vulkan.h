#ifndef _GAZOLAND_SRC_GAZOLAND_VULKAN_H_
#define _GAZOLAND_SRC_GAZOLAND_VULKAN_H_

#include <vulkan/vulkan.h>

#define CHECK_GL()

class program_info {
 public:
  program_info() = default;
  ~program_info();
  
  void link(
    GLuint vertex_shader, GLuint fragment_shader,
    const char* u_panning_name, const char* u_projection_name, const char* u_texture_name,
    const char* v_pos_name, const char* v_uv_name);

  std::string name;
};

#endif  // #ifndef _GAZOLAND_SRC_GAZOLAND_VULKAN_H_
