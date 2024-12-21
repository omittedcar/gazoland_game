#include "gazo.h"
class level {
 public:
  void construct();
  void demolish();
  void time_step();
  void control_gazo(float left_stick_x, float left_stick_y, float right_stick_x, float right_stick_y);
  void draw(shader* gazo_shader, GLuint gazo_texture);
 private:
  gazo the_gazo;
};