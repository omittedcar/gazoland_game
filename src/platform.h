#include "gles_or_vulkan.h"

#ifndef THE_MECHANISM_SRC_PLATFORM_H
#define THE_MECHANISM_SRC_PLATFORM_H

class platform {
 public:
  void arise(std::vector<fvec2> corners_in, int side_count_in);

  bool can_we_like_can_we_please_like_put_stuff_here_at_this_location_x_and_y_please_or_is_that_like_a_not_good_place_to_put_stuff_because_like_you_cant_put_stuff_there(vec2 p);

  vec2 shortest_path(vec2 p);

  void draw(
    const std::shared_ptr<program>& surface_shader,
    const std::shared_ptr<program>& fill_shader,
    const std::vector<float>& projection_matrix,
    const fvec2& view);

 private:
  int side_count;
  std::vector<fvec2> corners;
  fvec2 bounding_box[2];
  std::shared_ptr<buffer> vertex_uv_buffer;
  std::shared_ptr<buffer> vertex_pos_buffer;
  std::shared_ptr<buffer> corner_vertex_buffer;
  std::shared_ptr<buffer> upper_surface_index_buffer;
  std::shared_ptr<buffer> lower_surface_index_buffer;
  std::shared_ptr<buffer> inner_face_index_buffer;
  void compute_bounding_box();
  void do_vertex_buffers();
  void generate_mesh();
};
#endif
