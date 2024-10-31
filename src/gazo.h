#include "stdlib.h"
#include <GLES3/gl3.h>

class gazo {
 public:
  void init();
  bool advance_forward(double time_step);
  void update_gl_vertex_buffer();
  GLuint get_gl_vertex_buffer();
  ushort* get_element_pointer();
  double* get_mapping_pointer();
  int get_vertex_buffer_size();
  void kill_to_death();

 private:

  double pointing[2] = {
    0.0f,
    0.0f
  };

  double* mapping;
  double* pos;
  double* vel;

  double* delta_pos;
  double* delta_vel;
  double* sample_vel;
  double* sample_pos;
  double* acc;

  double* muscle_forces;

  float* pos20;
  ushort* elements;
  GLuint gl_vertex_buffer;



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

  void calculate_acc(
    double* pos_in,
    double* vel_in,
    double* acc_out
  );

  bool is_in_wall();
  void push_out_from_wall(double time_since_not_in_wall);
};
