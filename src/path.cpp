#ifdef _WIN32
#include <windows.h>
#endif

#include "path.h"
#include <stringapiset.h>
#include <winnls.h>

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
    rp = std::filesystem::path(dirname(exe));
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

std::string path_to_str(const std::filesystem::path& path) {
#ifdef __linux__
  return path.c_str();
#endif

#ifdef _WIN32
  std::wstring w_str(path.c_str());
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, &w_str[0], (int)w_str.size(), NULL, 0, NULL, NULL);
  std::string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &w_str[0], (int)w_str.size(), &strTo[0], size_needed, NULL, NULL);
  return strTo;
#endif  
}
