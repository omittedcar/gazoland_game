#include "shader.h"

#ifndef THE_MECHANISM_SRC_PLATFORM_H
#define THE_MECHANISM_SRC_PLATFORM_H

class platform {
 public:
  void arise();
  void demolish();
  void draw(
    shader* rendering_shader,
    float projection_matrix[20],
    float view[3],
    GLuint texture
  );
 private:
  int side_count;
  float* corners;
  float bounding_box[4];
  GLuint vertex_buffer;
  void compute_bounding_box();};
#endif