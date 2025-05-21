#include "gl_program_info.h"
#include <stdlib.h>
#include <stdio.h>
void gl_program_info::link(
    GLuint vertex_shader, GLuint fragment_shader,
    const char *u_panning_name, const char *u_projection_name, const char *u_texture_name,
    const char *v_pos_name, const char *v_uv_name)
{
  program = glCreateProgram();
  //glBindUniformLocation(shader, u_panning = 0, u_panning_name);
  //glBindUniformLocation(shader, u_projection = 1, u_projection_name);
  //glBindUniformLocation(shader, u_texture = 2, u_texture_name);
  glBindAttribLocation(program, v_pos = 0, v_pos_name);
  glBindAttribLocation(program, v_uv = 1, v_uv_name);
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  //u_pos = 0;
  //v_uv = 1;
  if(  u_panning_name != nullptr)   u_panning = glGetUniformLocation(program, u_panning_name   );
  if(u_projection_name!= nullptr)u_projection = glGetUniformLocation(program, u_projection_name);
  if(  u_texture_name != nullptr)   u_texture = glGetUniformLocation(program, u_texture_name   );
  // if(      v_pos_name != nullptr)       v_pos = glGetAttribLocation(shader, v_pos_name       );
  // if(       v_uv_name != nullptr)        v_uv = glGetAttribLocation(shader, v_uv_name        );
}
gl_program_info::~gl_program_info() {
  if (program) {
    glDeleteProgram(program);
  }
}