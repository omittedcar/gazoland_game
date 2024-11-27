#include "gazo.h"
#include "shader.h"

#include <linux/input.h>

class GLFWwindow;

class game {
 public:
  game() = default;

  void run();
  void stop();

 private:
  void the_monitor_has_refreshed_again();
  void function_which_is_called_420hz();

  input_event rumbleinator;
	input_event derumbleinator;
	ff_effect rumble_effect;
  int rumbly_file_descriptor;
  int joystick_file_descriptor;
  float aspect = 1.5;
  float fov = 1.0;
  float near = 0.01; // I love z-fighting :)
  float projection_matrix[020] = {
    1/aspect/fov, 0, 0, 0,
    0, 1/fov, 0, 0,
    0, 0, 1, 1,
    0, 0, -1, 0
  };
  float view[3] = {0};
  int frame_counter = 0;
  gazo the_gazo;
  bool is_playing = false;
  GLFWwindow *window = nullptr;
  int window_width = 0;
  int window_height = 0;
  shader the_shader;
  char* info_log = nullptr;
};
