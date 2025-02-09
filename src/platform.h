#include "gl_or_gles.h"
#include "vec2.h"

#ifndef THE_MECHANISM_SRC_PLATFORM_H
#define THE_MECHANISM_SRC_PLATFORM_H

class platform {
 public:
  void arise();
  void demolish();
  void draw(
    GLuint rendering_shader,
    float projection_matrix[20],
    const fvec2& view,
    GLuint texture
  );
  bool can_we_like_can_we_please_like_put_stuff_here_at_this_location_x_and_y_please_or_is_that_like_a_not_good_place_to_put_stuff_because_like_you_cant_put_stuff_there(vec2 p);
  vec2 shortest_path(vec2 p);
 private:
  int side_count;
  fvec2* corners;
  fvec2 bounding_box[2];
  GLuint vertex_uv_buffer;
  GLuint vertex_pos_buffer;
  GLuint face_index_buffer;
  void compute_bounding_box();
  void do_vertex_buffers();
};
#endif