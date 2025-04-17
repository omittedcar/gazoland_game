#ifndef THE_MECHANISM_SRC_GL_PROGRAM_INFO_DEF
#define THE_MECHANISM_SRC_GL_PROGRAM_INFO_DEF
#include "gl_or_gles.h"

struct gl_program_info {
  void link(
    GLuint vertex_shader, GLuint fragment_shader,
    const char* u_panning_name, const char* u_projection_name, const char* u_texture_name,
    const char* v_pos_name, const char* v_uv_name);

  GLuint shader;
  GLint u_panning;
  GLint u_projection;
  GLint u_texture;
  GLint v_pos;
  GLint v_uv;
};
#endif