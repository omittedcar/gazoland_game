#include "level.h"
#include <GLES3/gl3.h>

void level::construct() {
  the_gazo.init();
  the_platform.arise();
}
void level::demolish() {
  the_gazo.kill_to_death();
  the_platform.demolish();
}
void level::time_step() {
  the_gazo.advance_forward(1.0/480.0);
  the_gazo.push_out_from_platform(1.0/480.0, &the_platform);
}

void level::control_gazo(float left_stick_x, float left_stick_y, float right_stick_x, float right_stick_y) {
  the_gazo.point_joystick(left_stick_x, left_stick_y);
  the_gazo.point_other_joystick(right_stick_x,right_stick_y);
}

void level::draw(
  GLuint gazo_shader, GLuint terrain_shader,
  GLuint gazo_texture, GLuint stone_tile_texture
) {
  view = the_gazo.get_center_of_mass_medium_precision();

  glClearColor(
    0.0,
    0.0,
    0.0,
    1.0
  );
  glClearDepthf(1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  float aspect = 1.5;
  float fov = 4.0;
  float near = 0.01;
  float projection_matrix[020] = {
    1/aspect/fov, 0, 0, 0,
    0, 1/fov, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
  };
  the_gazo.render(
    gazo_shader,
    projection_matrix,
    view,
    gazo_texture
  );
  the_platform.draw(
    terrain_shader,
    projection_matrix,
    view,
    stone_tile_texture
  );

}