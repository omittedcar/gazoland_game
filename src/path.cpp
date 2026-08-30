#include "path.h"

#ifdef __linux__
#include <libgen.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <libloaderapi.h>
#include <PathCch.h>
#endif

namespace { 
  std::filesystem::path rp;
}

std::filesystem::path root_path() {
  if (rp.empty()) {
#ifdef __linux__
    char exe[256];
    readlink("/proc/self/exe", exe, 256);
    rp = std::filesystem::path(dirname(dirname(exe)));
#endif

#ifdef _WIN32
    WCHAR exe[256];
    GetModuleFileNameW(nullptr, exe, 256);
    PathCchRemoveFileSpec(exe, 256);
    rp = std::filesystem::path(exe);
#endif
  }
  return rp;
}
