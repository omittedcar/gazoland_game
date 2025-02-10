#ifndef THE_MECHANISM_SRC_GL_PROGRAM_INFO_DEF
#define THE_MECHANISM_SRC_GL_PROGRAM_INFO_DEF
#include "gl_or_gles.h"
struct gl_program_info {
  GLuint shader;
  GLint u_panning;
  GLint u_projection;
  GLint u_texture;
  GLint v_pos;
  GLint v_uv;
};
#endif