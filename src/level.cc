#include "level.h"
#include "gl_or_gles.h"

void level::construct() {
  the_gazo.init();
  the_platform.arise();
}
void level::demolish() {
  the_gazo.kill_to_death();
  the_platform.demolish();
}
void level::time_step() {
  the_gazo.advance_forward(1.0 / 480.0);
  the_gazo.push_out_from_platform(1.0 / 480.0, &the_platform);
}

void level::control_gazo(float left_stick_x, float left_stick_y,
                         float right_stick_x, float right_stick_y) {
  the_gazo.point_joystick(left_stick_x, left_stick_y);
  the_gazo.point_other_joystick(right_stick_x, right_stick_y);
}

void level::draw(gl_program_info* gazo_shader, gl_program_info* terrain_shader,
  GLuint gazo_texture, GLuint stone_tile_texture) {
  view = the_gazo.get_center_of_mass_medium_precision();
  glClearColor(0.0625, 0.0625, 0.0625, 1.0);
  glClearDepthf(1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  float aspect = 1.5;
  float fov = 4.0;
  float projection_matrix[020] = {
      1 / aspect / fov, 0, 0, 0, 0, 1 / fov, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  glDisable(GL_BLEND);
   
  glUseProgram(gazo_shader->shader);
  glUniformMatrix4fv(
    gazo_shader->u_projection, 1, GL_FALSE, projection_matrix
  );  
  glUniform2f(
    gazo_shader->u_panning, view.x, view.y
  );
  glUniform1i(
    gazo_shader->u_texture,0
  );
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, gazo_texture);
  the_gazo.render(gazo_shader);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glUseProgram(terrain_shader->shader);
  glUniformMatrix4fv(
    terrain_shader->u_projection, 1, GL_FALSE, projection_matrix
  );
  glUniform2f(
    terrain_shader->u_panning, view.x, view.y
  );
  glUniform1i(
    terrain_shader->u_texture, 0
  );
  glBindTexture(GL_TEXTURE_2D, stone_tile_texture);
  the_platform.draw(terrain_shader);
}