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
  k_array,
  k_uniform
};

#ifdef GAZOLAND_GLES
# include "gazoland_gles.h"
#endif

#ifdef GAZOLAND_VULKAN
# include "gazoland_vulkan.h"
#endif

#endif  // #ifdef _GAZOLAND_SRC_GLES_OR_VULKAN_H_
