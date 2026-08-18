#ifndef _GAZOLAND_SRC_GAME_H_
#define _GAZOLAND_SRC_GAME_H_

#include "gazo.h"
#include "level.h"

#include "gles_or_vulkan.h"

#include <memory>
//#include <linux/input.h>
#include <filesystem>

class GLFWwindow;


  static constexpr size_t k_ui_size = 0x9000;

  enum class gamestate {
    kLoading,
    kTitleScreen,
    kPlaying,
    kCleanup
  };
  
  void run();
  void the_monitor_has_refreshed_again();
  void load();
  void update();
  void function_which_is_called_480hz();
  void unload();


#endif  // #ifdef _GAZOLAND_SRC_GAME_H_
