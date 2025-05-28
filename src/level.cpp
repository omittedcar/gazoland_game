#include "level.h"
#include "gles_or_vulkan.h"

#include <iostream>
#include <math.h>
#include <tuple>

level::level(
    const char* file_name,
    std::shared_ptr<gazo>& gazo_arg,
    std::shared_ptr<program>& terrain_prog_arg,
    std::shared_ptr<program>& polygon_fill_prog_arg,
    std::shared_ptr<texture>& stone_tile_tex_arg)
    : the_gazo(gazo_arg),
      the_platform(file_name,
                   terrain_prog_arg,
                   polygon_fill_prog_arg,
                   stone_tile_tex_arg) {}

void level::time_step() {
  the_gazo->advance_forward(1.0 / 480.0);
  the_gazo->push_out_from_platform(1.0 / 480.0, the_platform);
}

void level::control_gazo(float left_stick_x, float left_stick_y,
                         float right_stick_x, float right_stick_y) {
  the_gazo->point_joystick(left_stick_x, left_stick_y);
  the_gazo->point_other_joystick(right_stick_x, right_stick_y);
}

void level::draw(const std::vector<float>& projection,
                 const fvec2& view) {
  the_platform.draw(projection, view);
}
