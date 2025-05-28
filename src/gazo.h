#ifndef _GAZOLAND_SRC_GAZO_H_
#define _GAZOLAND_SRC_GAZO_H_

#include "stdlib.h"
#include "gles_or_vulkan.h"
#include "platform.h"

class gazo {
 public:
  gazo(std::shared_ptr<program> prog_arg,
       std::shared_ptr<texture> spritesheet_tex_arg);

  const std::shared_ptr<program>& get_prog() const { return prog; }
  const std::shared_ptr<texture>& get_tex() const { return spritesheet_tex; }
  const std::shared_ptr<buffer>& get_vertex_buf() const {
    return vertex_buffer;
  }
  const std::shared_ptr<buffer>& get_uv_buf() const { return uv_buffer; }
  const std::shared_ptr<buffer>& get_element_index_buf() const {
    return element_index_buffer;
  }
  int get_uv_map_offset() const { return uv_map_offset; }
  int get_n_sides() const { return 15; }

  bool advance_forward(double time_step);
  void point_joystick(float x, float y);
  void point_other_joystick(float x, float y);
  int get_vertex_buffer_size();
  float get_rumble();
  fvec2 get_center_of_mass_medium_precision();
  void push_out_from_platform(double interval, platform& p);

  void update_vertex_buffer();
  void update_uv_buffer();
  void choose_sprite();

  void draw(const std::vector<float>& projection, fvec2 view);
  
 private:
  std::shared_ptr<program> prog;
  std::shared_ptr<texture> spritesheet_tex;

  vec2 pointing{0.0f, 0.0f};
  vec2 previous_joystick{0.0, 0.0};

  std::vector<vec2> mapping;
  std::vector<vec2> pos;
  std::vector<vec2> vel;

  std::vector<vec2> delta_pos;
  std::vector<vec2> delta_vel;
  std::vector<vec2> sample_vel;
  std::vector<vec2> sample_pos;
  std::vector<vec2> acc;

  std::vector<float> pos20;
  int uv_map_offset;

  std::shared_ptr<buffer> vertex_buffer;
  std::shared_ptr<buffer> uv_buffer;
  std::shared_ptr<buffer> element_index_buffer;

  void calculate_acc(
    const std::vector<vec2>& pos_in,
    const std::vector<vec2>& vel_in,
    std::vector<vec2>& acc_out
  );
};

#endif  // #ifndef _GAZOLAND_SRC_GAZO_H_
