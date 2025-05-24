#ifndef _GAZOLAND_SRC_LEVEL_H_
#define _GAZOLAND_SRC_LEVEL_H_

#include "gazo.h"
#include "platform.h"

#include "gles_or_vulkan.h"

#include <stdio.h>
#include <stdlib.h>

class level {
public:
  level(const char* file_name, std::shared_ptr<gazo>& gazo_arg);
  ~level() = default;
  
  void time_step();
  void control_gazo(float left_stick_x, float left_stick_y, float right_stick_x,
                    float right_stick_y);
  void draw(
      const std::shared_ptr<program>& terrain_shader,
      const std::vector<float>& projection_matrix,
      const fvec2& view,
      std::shared_ptr<texture>& stone_tile_texture,
      std::shared_ptr<program>& polygon_fill_shader);

private:
  std::shared_ptr<gazo> the_gazo;
  platform the_platform;
};

#endif  // #ifndef _GAZOLAND_SRC_LEVEL_H_

