#include "./game.h"
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>



int main(int argc, char *argv[]) {
  //open the directory
  DIR *game_directory;

  game_directory = opendir(".");
  game the_game;
  the_game.game_directory = game_directory;
  the_game.run();
  closedir(game_directory);
  return 0;
}
