
#include "level.h"
#include <linux/input.h>
#include "gl_program_info.h"

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
  GLuint vertshader_no_uv_map;
  GLuint fragshader_basic;
  GLuint fragshader_gamma;

  GLuint gamma_shader;
  gl_program_info gazo_shader_info;
  gl_program_info terrain_shader_info;
  gl_program_info polygon_fill_shader_info;

  GLuint square_buffer;
  GLuint gazo_spritesheet_texture;
  GLuint stone_tile_texture;
  GLuint framebuffer;
  GLuint framebuffer_texture;
  GLuint depth_texture;
};
