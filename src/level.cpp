#include "level.h"
#include "path.h"
#include "gl_or_gles.h"

#include <iostream>
#include <fstream>
#include <math.h>
void level::construct(const char* file_name) {
  std::filesystem::path full_path(root_path());
  full_path /= "assets";
  full_path /= "levels";
  full_path /= file_name;
  std::ifstream ifs(full_path.string(), std::ios::in);
  ifs >> std::noskipws;
  ifs.seekg(3);
  unsigned char level_size;
  
  ifs >> level_size;
  printf("the level is %i big \n", level_size);
  fvec2* platform_corners = (fvec2*) malloc(level_size * sizeof(fvec2));
  //platform_corners[0] = {-0.9, -1.0};
  //platform_corners[1] = {-1.0, -1.1};
  //platform_corners[2] = {-1.0, -1.9};
  //platform_corners[3] = {-0.9, -2.0};
  //platform_corners[4] = {+0.9, -2.0};
  //platform_corners[5] = {+1.0, -1.9};
  //platform_corners[6] = {+1.0, -1.1};
  //platform_corners[7] = {+0.9, -1.0};

  
  //this gets freed by the platform bc im a bad coder

  for(int i = 0; i < level_size; i++) {
    unsigned char the_char;
    ifs >> the_char;
    platform_corners[i].x = float((int(the_char) - 0x80) * 4);
    ifs >> the_char;
    platform_corners[i].x += float(the_char) / 64.0;
    ifs >> the_char;
    platform_corners[i].y = float((int(the_char) - 0x80) * 4);
    ifs >> the_char;
    platform_corners[i].y += float(the_char) / 64.0;
  }

  the_gazo.init();
  the_platform.arise(platform_corners, level_size);
}
void level::demolish() {
  fclose(the_file);
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

void level::draw(
  gl_program_info* gazo_shader, gl_program_info* terrain_shader,
  gl_program_info* polygon_fill_shader,
  GLuint gazo_texture, GLuint stone_tile_texture
) {
  view = the_gazo.get_center_of_mass_medium_precision();
  glClearColor(0.5, 0.5, 0.5, 1.0);
  glClearDepthf(1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  float aspect = 4.0/3.0;
  float view_area = 16.0;
  float projection_matrix[020] = {
      2 * sqrt(1/aspect / view_area), 0, 0, 0,
      0, 2 * sqrt(aspect / view_area), 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1};
  glDisable(GL_BLEND);
   
  glUseProgram(gazo_shader->program);
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
  glBlendFunc(GL_ONE, GL_SRC_ALPHA);
  glUseProgram(terrain_shader->program);
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
  the_platform.draw(terrain_shader, polygon_fill_shader, projection_matrix, view);
}
