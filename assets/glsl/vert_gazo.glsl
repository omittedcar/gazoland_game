#version 310 es

precision highp float;
layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 vert_uv;
layout(binding = 0) uniform Projection {
  vec2 view;
  mat4 matrix;
} projection;
layout(location = 0) out vec2 uv;
void main() {
  uv = vert_uv;
  gl_Position = projection.matrix * vec4((pos - projection.view), 0.25, 1.0);
}