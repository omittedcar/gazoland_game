#include "gl_program_info.h"

#include <iostream>

namespace {

GLint get_uniform(const gl_program_info& info, const GLchar* name) {
  if (name == nullptr) {
    return -1;
  }
  GLint result = glGetUniformLocation(info.program, name);
  CHECK_GL();
  if (result == -1) {
    std::cerr << "glGetUniformLocation(\"" << info.name << "\", \"" << name
	      << "\") failed." << std::endl;
  }
  return result;
}

GLint get_attribute(const gl_program_info& info, const GLchar* name) {
  if (name == nullptr) {
    return -1;
  }
  GLint result = glGetAttribLocation(info.program, name);
  CHECK_GL();
  if (result == -1) {
    std::cerr << "glGetAttribLocation(\"" << info.name << "\", \"" << name
	      << "\") failed." << std::endl;
  }
  return result;
}

}  // namespace {

gl_program_info::gl_program_info(const char* name_arg)
  : name(name_arg) {}

gl_program_info::~gl_program_info() {
  if (program) {
    glDeleteProgram(program);
  }
}

// static
bool gl_program_info::maybe_print_error(const char *file, size_t line) {
  GLenum err;
  bool result = false;
  while ((err = glGetError()) != GL_NO_ERROR)
  {
    result = true;
    std::cerr << "GL error code 0x" << std::hex << err << std::dec
	      << " prior to " << file << ":" << line << std::endl;
  }
  return result;
}

void gl_program_info::link(
    GLuint vertex_shader, GLuint fragment_shader,
    const char* u_panning_name, const char* u_projection_name,
    const char* u_texture_name, const char* v_pos_name, const char* v_uv_name) {
  program = glCreateProgram();
  CHECK_GL();
  glAttachShader(program, vertex_shader);
  CHECK_GL();
  glAttachShader(program, fragment_shader);
  CHECK_GL();
  glLinkProgram(program);
  CHECK_GL();
  GLint params;
  glGetProgramiv(program, GL_LINK_STATUS, &params);
  CHECK_GL();
  if (params != GL_TRUE) {
    std::cerr << "Link failed for \"" << name << "\"." << std::endl;
  }
  
  u_panning = get_uniform(*this, u_panning_name);
  u_projection = get_uniform(*this, u_projection_name);
  u_texture = get_uniform(*this, u_texture_name);
  v_pos = get_attribute(*this, v_pos_name);
  v_uv = get_attribute(*this, v_uv_name);
}
