
#include "level.h"
#include <linux/input.h>

class GLFWwindow;

class game {
 public:
  game() = default;

  void run();
  void stop();

 private:
  
  void the_monitor_has_refreshed_again();
  void function_which_is_called_480hz();

  input_event rumbleinator;
	input_event derumbleinator;
	ff_effect rumble_effect;
  int rumbly_file_descriptor;
  int joystick_file_descriptor;
  int frame_counter = 0;
  level the_level;
  bool is_playing = false;
  GLFWwindow *window = nullptr;
  int window_width = 0;
  int window_height = 0;
  char* info_log = nullptr;
  GLuint vertshader_basic;
  GLuint vertshader_gazo;
  GLuint vertshader_3d;
  GLuint fragshader_basic;
  GLuint fragshader_gamma;
  GLuint gamma_shader;
  GLuint gazo_shader;
  GLuint gazo_shader_u_view;
  GLuint gazo_shader_u_projection;
  GLuint gazo_shader_u_texture;
  GLuint terrain_shader;
  GLuint terrain_shader_;
  GLuint terrain_shader_u_view_pos;
  GLuint terrain_shader_u_projection_matrix;
  GLuint square_buffer;
  GLuint gazo_spritesheet_texture;
  GLuint stone_tile_texture;
  GLuint framebuffer;
  GLuint framebuffer_texture;
  GLuint depth_texture;
};
