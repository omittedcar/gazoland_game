#ifndef THE_MECHANISM_SRC_GL_PROGRAM_INFO_DEF
#define THE_MECHANISM_SRC_GL_PROGRAM_INFO_DEF
#include "gl_or_gles.h"

#include <cstddef>
#include <string>

#define CHECK_GL() gl_program_info::maybe_print_error(__FILE__, __LINE__)

class gl_program_info {
 public:
  ~gl_program_info();

  static bool maybe_print_error(const char *file, size_t line);

  void link(
    GLuint vertex_shader, GLuint fragment_shader,
    const char* u_panning_name, const char* u_projection_name, const char* u_texture_name,
    const char* v_pos_name, const char* v_uv_name);

  std::string name;
  GLuint program;
  GLint u_panning = -1;
  GLint u_projection = -1;
  GLint u_texture = -1;
  GLint v_pos = -1;
  GLint v_uv = -1;
};
#endif
