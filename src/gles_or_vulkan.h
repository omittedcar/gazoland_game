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
# define GLFW_INCLUDE_VULKAN
#endif

#include <GLFW/glfw3.h>

#endif  // #ifdef _GAZOLAND_SRC_GLES_OR_VULKAN_H_
