
#include "level.h"
#include <linux/input.h>
#include "gl_program_info.h"
#include <dirent.h>
#include <memory>

class GLFWwindow;

class game {
 public:
  game() = default;
  void run();
  void stop();
  DIR *game_directory;

 private:
  
  void the_monitor_has_refreshed_again();
  void function_which_is_called_480hz();

  void write_text(const char* the_text_which_we_are_writing, int pos);

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

  unsigned char* lettering;

  GLuint vertshader_basic;
  GLuint vertshader_gazo;
  GLuint vertshader_3d;
  GLuint vertshader_no_uv_map;
  GLuint fragshader_basic;
  GLuint fragshader_gamma;
  GLuint fragshader_gui;

  GLuint gamma_shader;
  gl_program_info gazo_shader_info;
  gl_program_info terrain_shader_info;
  gl_program_info polygon_fill_shader_info;
  gl_program_info gui_shader_info;

  GLuint square_buffer;
  GLuint gazo_spritesheet_texture;
  GLuint stone_tile_texture;
  GLuint bailey_truss_texture;
  GLuint framebuffer;
  GLuint framebuffer_texture;
  GLuint depth_texture;
  GLuint gui_texture;
  void do_shader(const char* name, int shader_type);
};
