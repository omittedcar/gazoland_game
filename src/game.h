#ifndef _GAZOLAND_SRC_GAME_H_
#define _GAZOLAND_SRC_GAME_H_

#include "gazo.h"
#include "level.h"

#include "gles_or_vulkan.h"

#include <memory>
#include <linux/input.h>
#include <filesystem>

class GLFWwindow;

class game {
 public:
  static constexpr size_t k_ui_size = 0x9000;

  game() {}
  ~game();
  void run();

 private:
  void the_monitor_has_refreshed_again();
  void load();
  void update();
  void function_which_is_called_480hz();
  void unload();
#define LOADING 0x0
#define TITLE_SCREEN 0x01
#define PLAYING 0x02
#define CLEANUP 0x03
  unsigned char state = LOADING;
  input_event rumbleinator;
  input_event derumbleinator;
  ff_effect rumble_effect;
  int rumbly_file_descriptor;
  int joystick_file_descriptor;
  int frame_counter = 0;
  std::shared_ptr<gazo> the_gazo;
  std::unique_ptr<level> the_level;
  bool is_playing = false;
  GLFWwindow* window = nullptr;

  unsigned char* lettering;

  std::shared_ptr<program> gazo_prog;
  std::shared_ptr<program> terrain_prog;
  std::shared_ptr<program> polygon_fill_prog;
  std::shared_ptr<program> gui_prog;
  std::shared_ptr<program> gamma_prog;

  std::shared_ptr<texture> gazo_spritesheet_tex;
  std::shared_ptr<texture> stone_tile_tex;
  std::shared_ptr<texture> bailey_truss_tex;

  std::shared_ptr<buffer> square_buf;
  std::shared_ptr<framebuffer> draw_fb;

  std::shared_ptr<texture> draw_tex;
  std::shared_ptr<texture> depth_tex;
  std::shared_ptr<texture> gui_tex;
};

#endif  // #ifdef _GAZOLAND_SRC_GAME_H_
