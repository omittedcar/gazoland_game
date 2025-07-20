#version 310 es
precision highp float;
in vec3 pos;
in vec2 vertex_uv;
uniform vec2 view_pos;
uniform mat4 projection_matrix;
out vec2 uv;
void main() {
  uv = vertex_uv;
  vec2 displeysmant = pos.xy - view_pos;
  gl_Position = projection_matrix * vec4(displeysmant, pos.z, 1.);
}