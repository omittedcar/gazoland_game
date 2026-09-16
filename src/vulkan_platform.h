#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#ifndef THE_MECHANISM_SRC_PLATFORM_H
#define THE_MECHANISM_SRC_PLATFORM_H

class platform {
 public:
  void arise(glm::vec2* corners_in, int side_count_in);
  void demolish();
  void draw(
    //gl_program_info* surface_shader,
    //gl_program_info* fill_shader,
    float* projection,
    glm::vec2 view
  );
  bool can_we_like_can_we_please_like_put_stuff_here_at_this_location_x_and_y_please_or_is_that_like_a_not_good_place_to_put_stuff_because_like_you_cant_put_stuff_there(glm::dvec2 p);
  glm::dvec2 shortest_path(glm::dvec2 p);
  //GLuint outer_texture;
  //GLuint inner_texture;
 private:
  int side_count;
  glm::vec2* corners;
  glm::vec2 bounding_box[2];
  //GLuint vertex_uv_buffer;
  //GLuint vertex_pos_buffer;
  //GLuint corner_vertex_buffer;
  //GLuint upper_surface_index_buffer;
  //GLuint lower_surface_index_buffer;
  //GLuint inner_face_index_buffer;
  void compute_bounding_box();
  void do_vertex_buffers();
  void generate_mesh();
};
#endif