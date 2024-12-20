#include "level.h"

void level::construct() {
  the_gazo.init();
}
void level::demolish() {
  the_gazo.kill_to_death();
}
void level::time_step() {
  the_gazo.advance_forward(1.0/480.0);
}

void level::draw(shader* gazo_shader, GLuint gazo_texture) {
  float aspect = 1.5;
  float fov = 1.0;
  float near = 0.01;
  float projection_matrix[020] = {
    1/aspect/fov, 0, 0, 0,
    0, 1/fov, 0, 0,
    0, 0, 1, 1,
    0, 0, -1, 0
  };
  float view[3] = {
    0, 0.0, -2.0
  };
  the_gazo.render(
    gazo_shader,
    projection_matrix,
    view,
    gazo_texture
  );
}