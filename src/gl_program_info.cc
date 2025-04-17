#include "gl_program_info.h"
void gl_program_info::link(
    GLuint vertex_shader, GLuint fragment_shader,
    const char* u_panning_name, const char* u_projection_name, const char* u_texture_name,
    const char* v_pos_name, const char* v_uv_name) {
  shader = glCreateProgram();
  glAttachShader(shader, vertex_shader);
  glAttachShader(shader, fragment_shader);
  glLinkProgram(shader);
  if(  u_panning_name != nullptr)   u_panning = glGetUniformLocation(shader, u_panning_name   );
  if(u_projection_name!= nullptr)u_projection = glGetUniformLocation(shader, u_projection_name);
  if(  u_texture_name != nullptr)   u_texture = glGetUniformLocation(shader, u_texture_name   );
  if(      v_pos_name != nullptr)       v_pos =  glGetAttribLocation(shader, v_pos_name       );
  if(       v_uv_name != nullptr)        v_uv =  glGetAttribLocation(shader, v_uv_name        );
}