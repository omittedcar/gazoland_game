#include "stdlib.h"
#include <GLES3/gl3.h>
class shader;

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
  void render(
    shader* rendering_shader,
    float projection_matrix[20],
    float view[3],
    GLuint texture
  );
 private:
  double pointing[2] = {
    0.2f,
    0.2f
  };
  double previous_joystick[2] = {
    0.0,
    0.0
  };

  double* mapping;
  double* pos;
  double* vel;

  double* delta_pos;
  double* delta_vel;
  double* sample_vel;
  double* sample_pos;
  double* acc;


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
    double* pos_in,
    double* vel_in,
    double* acc_out
  );

  bool is_in_wall();
  void push_out_from_wall(double time_since_not_in_wall);
};
