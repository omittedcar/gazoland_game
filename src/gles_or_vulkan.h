#ifndef _GAZOLAND_SRC_GLES_OR_VULKAN_H_
#define _GAZOLAND_SRC_GLES_OR_VULKAN_H_

enum class shader_type {
  k_vertex,
  k_fragment,
};
  
enum class texture_type {
  k_2d
};

enum class buffer_type {
  k_array
};

#ifdef GAZOLAND_GLES
# include "gazoland_gles.h"
#endif

#ifdef GAZOLAND_VULKAN
# include "gazoland_vulkan.h"
# ifdef _WIN32
#  define VK_USE_PLATFORM_WAYLAND_KHR
# else
#  define VK_USE_PLATFORM_WAYLAND_KHR
# endif
# define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <GLFW/glfw3.h>

#ifdef GAZOLAND_VULKAN
# ifdef _WIN32
#  define GLFW_EXPOSE_NATIVE_WIN32
# else
#  define GLFW_EXPOSE_NATIVE_WAYLAND
# endif
# include <GLFW/glfw3native.h>
#endif

#endif  // #ifdef _GAZOLAND_SRC_GLES_OR_VULKAN_H_
