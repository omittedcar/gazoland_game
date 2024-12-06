#include <GLES3/gl3.h>

#include <stdlib.h>
#include <stdio.h>

#include "./shader.h"

void shader::init(const char* vertex_shader_source_code, const char* fragment_shader_source_code){ // shaders get compiled from source code at runtime 😝
  info_log = (char*) malloc(04000);
  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(
    vertex_shader,
    1,
    &vertex_shader_source_code,
    nullptr
  );
  glCompileShader(vertex_shader);
  glGetShaderInfoLog(vertex_shader, 01000, nullptr, info_log);
  //printf("\n%s\n%s\n", vertex_shader_source_code, info_log);
  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(
    fragment_shader,
    1,
    &fragment_shader_source_code,
    nullptr
  );
  glCompileShader(fragment_shader);
  glGetShaderInfoLog(fragment_shader, 01000, nullptr, info_log);
  //printf("%s\n%s\n", fragment_shader_source_code, info_log);
  shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);
  pos_loc = glGetAttribLocation(shader_program, "pos");
  projection_loc = glGetUniformLocation(shader_program, "projection_matrix");
  view_loc = glGetUniformLocation(shader_program, "view_pos");
}

GLuint shader::get_pos_loc() {
  return pos_loc;
}

GLuint shader::get_projection_loc() {
  return projection_loc;
}

GLuint shader::get_view_loc() {
  return view_loc;
}

GLuint shader::get_shader_program() {
  return shader_program;
}


void shader::put_away() {
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  glDeleteShader(shader_program);
  free(info_log);
}
