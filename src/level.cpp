#include "level.h"
#include "path.h"
#include "gles_or_vulkan.h"

#include <iostream>
#include <fstream>
#include <math.h>
#include <tuple>

level::level(const char* file_name, std::shared_ptr<gazo>& gazo_arg)
    : the_gazo(gazo_arg) {
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
  std::vector<fvec2> platform_corners(level_size);
  //fvec2* platform_corners = (fvec2*) malloc(level_size * sizeof(fvec2));
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

  the_platform.arise(std::move(platform_corners), level_size);
}

void level::time_step() {
  the_gazo->advance_forward(1.0 / 480.0);
  the_gazo->push_out_from_platform(1.0 / 480.0, the_platform);
}

void level::control_gazo(float left_stick_x, float left_stick_y,
                         float right_stick_x, float right_stick_y) {
  the_gazo->point_joystick(left_stick_x, left_stick_y);
  the_gazo->point_other_joystick(right_stick_x, right_stick_y);
}

void level::draw(
    const std::shared_ptr<program>& terrain_shader,
    const std::vector<float>& projection_matrix,
    const fvec2& view,
    std::shared_ptr<texture>& stone_tile_texture,
    std::shared_ptr<program>& polygon_fill_shader) {
  draw_level(
      terrain_shader, projection_matrix, view.x, view.y, stone_tile_texture);
  the_platform.draw(terrain_shader, polygon_fill_shader, projection_matrix, view);
}
