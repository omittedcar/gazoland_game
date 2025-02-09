#include "gazo.h"
#include "platform.h"
class level {
public:
  void construct(GLuint gazo_shader_u_view, GLuint gazo_shader_u_projection,
                 GLuint gazo_shader_u_texture, GLuint terrain_shader_u_view_pos,
                 GLuint terrain_shader_u_projection_matrix);
  void demolish();
  void time_step();
  void control_gazo(float left_stick_x, float left_stick_y, float right_stick_x,
                    float right_stick_y);
  void draw(GLuint gazo_shader, GLuint terrain_shader, GLuint gazo_texture,
            GLuint stone_tile_texture);

private:
  gazo the_gazo;
  platform the_platform;
  fvec2 view = {0.0, 0.0};
  GLuint gazo_shader_u_view_;
  GLuint gazo_shader_u_projection_;
  GLuint gazo_shader_u_texture_;
  GLuint terrain_shader_u_view_pos_;
  GLuint terrain_shader_u_projection_matrix_;
};