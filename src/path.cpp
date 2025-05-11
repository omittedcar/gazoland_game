#include "path.h"

namespace fs = std::filesystem;

const fs::path& root_path() {
  static fs::path root_path_;
  if (root_path_.empty()) {
    fs::path exe_link("/proc/self/exe");
    root_path_ = fs::read_symlink(exe_link).remove_filename().parent_path().parent_path();
  }
  return root_path_;
}
