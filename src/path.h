#ifndef GAZOLAND_GAME_PATH_H
#define GAZOLAND_GAME_PATH_H

#include <filesystem>

std::filesystem::path root_path();
std::string path_to_str(const std::filesystem::path& path);

#endif // #ifndef GAZOLAND_GAME_PATH_H