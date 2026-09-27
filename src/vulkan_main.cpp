#include <iostream>

#include "vulkan_app.h"

int main(int argc, char **argv) {
//  //open the directory
//  DIR *game_directory;
//
//  game_directory = opendir(".");
//  game the_game;
//  the_game.game_directory = game_directory;
//  the_game.run();
//  closedir(game_directory);
//  return 0;

  VulkanApp app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}