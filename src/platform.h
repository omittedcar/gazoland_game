#include "gles_or_vulkan.h"

#ifndef THE_MECHANISM_SRC_PLATFORM_H
#define THE_MECHANISM_SRC_PLATFORM_H

class platform {
 public:
  platform(
      const char *file_name,
      std::shared_ptr<program> terrain_prog,
      std::shared_ptr<program> polygon_fill_prog,
      std::shared_ptr<texture> stone_tile_tex);

  const std::shared_ptr<program>& get_terrain_prog() const {
    return terrain_prog;
  }
  const std::shared_ptr<program>& get_polygon_fill_prog() const {
    return polygon_fill_prog;
  }
  const std::shared_ptr<texture>& get_stone_tile_tex() const {
    return stone_tile_tex;
  }
  const std::shared_ptr<buffer>& get_vertex_uv_buffer() const {
    return vertex_uv_buffer;
  }
  const std::shared_ptr<buffer>& get_vertex_pos_buffer() const {
    return vertex_pos_buffer;
  }
  const std::shared_ptr<buffer>& get_corner_vertex_buffer() const {
    return corner_vertex_buffer;
  }
  const std::shared_ptr<buffer>& get_upper_surface_index_buffer() const {
    return upper_surface_index_buffer;
  }
  const std::shared_ptr<buffer>& get_lower_surface_index_buffer() const {
    return lower_surface_index_buffer;
  }
  const std::shared_ptr<buffer>& get_inner_face_index_buffer() const {
    return inner_face_index_buffer;
  }
  int get_side_count() const { return side_count; }
  const std::vector<fvec2>& get_corners() const { return corners; }
  
  bool can_we_like_can_we_please_like_put_stuff_here_at_this_location_x_and_y_please_or_is_that_like_a_not_good_place_to_put_stuff_because_like_you_cant_put_stuff_there(vec2 p);

  vec2 shortest_path(vec2 p);

  void draw(const std::vector<float>& projection, const fvec2& view);

 private:
  std::shared_ptr<program> terrain_prog;
  std::shared_ptr<program> polygon_fill_prog;
  std::shared_ptr<texture> stone_tile_tex;
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
