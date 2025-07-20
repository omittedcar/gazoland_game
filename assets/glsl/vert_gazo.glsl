#version 310 es

precision highp float;
in vec2 pos;
in vec2 vert_uv;
uniform vec2 view;
uniform mat4 projection;
out vec2 uv;
void main() {
  uv = vert_uv;
  gl_Position = projection * vec4((pos - view), 0.25, 1.0);
}