#include "path.h"
#include <unistd.h>
#include <libgen.h>

namespace { 
  std::filesystem::path rp;
}

std::filesystem::path root_path() {
  if (rp.empty()) {
    char exe[256];
    readlink("/proc/self/exe", exe, 256);
    rp = std::filesystem::path(dirname(dirname(exe)));      
  }
  return rp;
}
