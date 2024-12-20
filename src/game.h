
#include "shader.h"
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
  shader the_shader;
  char* info_log = nullptr;
  GLuint gazo_spritesheet_texture;
};
