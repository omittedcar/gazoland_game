#include "stdlib.h"
#include "gl_or_gles.h"
#include "platform.h"
#include "vec2.h"
#include "gl_program_info.h"

class gazo {
 public:
  void init();
  bool advance_forward(double time_step);
  void point_joystick(float x, float y);
  void point_other_joystick(float x, float y);
  void update_gl_vertex_buffer();
  GLuint get_gl_vertex_buffer();
  double* get_mapping_pointer();
  int get_vertex_buffer_size();
  void kill_to_death();
  float get_rumble();
  fvec2 get_center_of_mass_medium_precision();
  void render(
    gl_program_info* shader
  );
  void push_out_from_platform(double interval, platform* p);
 private:
  vec2 pointing = {
    0.0f,
    0.0f
  };
  vec2 previous_joystick = {
    0.0,
    0.0
  };
  int blink_timer = 0.0;

  vec2* mapping;
  vec2* pos;
  vec2* vel;

  vec2* delta_pos;
  vec2* delta_vel;
  vec2* sample_vel;
  vec2* sample_pos;
  vec2* acc;


  float* pos20;

  GLuint gl_vertex_buffer;
  GLuint gl_uv_buffer;
  GLuint gl_element_index_buffer;

  void add_thing_to_other_thing(
    double* thing,
    double* other_thing,
    double coefficient
  );

  void add_thing_to_other_thing_into_another_thing(
    double* thing,
    double* other_thing,
    double coefficient,
    double* another_thing
  );

  void update_gl_uv_buffer();

  void calculate_acc(
    vec2* pos_in,
    vec2* vel_in,
    vec2* acc_out
  );
};
