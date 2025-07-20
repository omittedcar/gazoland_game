#version 310 es
precision highp float;
in vec2 vertex_pos;
uniform vec2 view_pos;
uniform mat4 projection_matrix;
out vec2 uv;
void main() {
  uv = vertex_pos * -0.5;
  gl_Position = projection_matrix * vec4(vertex_pos - view_pos, 0.25, 1.0);
}