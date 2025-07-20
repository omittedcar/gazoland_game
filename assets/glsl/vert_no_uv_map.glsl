#version 310 es
precision highp float;
layout(location = 0) in vec2 vertex_pos;
struct Projection {
  vec2 view;
  mat4 matrix;
};
layout(location = 0) uniform Projection projection;
layout(location = 0) out vec2 uv;
void main() {
  uv = vertex_pos * -0.5;
  gl_Position = projection.matrix * vec4(vertex_pos - projection.view, 0.25, 1.0);
}