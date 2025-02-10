#include "gazo.h"
#include "platform.h"
#include "gl_program_info.h"
class level {
public:
  void construct();
  void demolish();
  void time_step();
  void control_gazo(float left_stick_x, float left_stick_y, float right_stick_x,
                    float right_stick_y);
  void draw(
    gl_program_info* gazo_shader, gl_program_info* terrain_shader,
    GLuint gazo_texture, GLuint stone_tile_texture
  );

private:
  gazo the_gazo;
  platform the_platform;
  fvec2 view = {0.0, 0.0};
};