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

struct vec2 {
  double x;
  double y;
  vec2& operator*=(float f) {
    x *= f;
    y *= f;
    return *this;
  }
  vec2& operator+=(const vec2& other) {
    x += other.x;
    y += other.y;
    return *this;
  }
};

struct fvec2 {
  float x;
  float y;
  fvec2& operator*=(float d) {
    x *= d;
    y *= d;
    return *this;
  }
  fvec2 operator*(float d) {
    return {x * d, y * d};
  }
  fvec2& operator+=(const fvec2& other) {
    x += other.x;
    y += other.y;
    return *this;
  }
};

struct svec2 {
  size_t width;
  size_t height;
};

inline vec2 operator+(const vec2& a, const vec2& b) {
  return {a.x + b.x, a.y + b.y};
}

inline fvec2 operator+(const fvec2& a, const fvec2& b) {
  return {a.x + b.x, a.y + b.y};
}

inline vec2 operator*(const vec2& a, double b) {
  return {a.x * b, a.y * b};
}

inline fvec2 operator*(const fvec2& a, float b) {
  return {a.x * b, a.y * b};
}

#endif  // #ifdef _GAZOLAND_SRC_GLES_OR_VULKAN_H_
