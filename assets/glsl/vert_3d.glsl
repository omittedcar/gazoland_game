#version 310 es
precision highp float;
layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 vertex_uv;
struct Projection {
  vec2 view;
  mat4 matrix;
};
layout(location = 0) uniform Projection projection;
layout(location = 0) out vec2 uv;
void main() {
  uv = vertex_uv;
  vec2 displeysmant = pos.xy - projection.view;
  gl_Position = projection.matrix * vec4(displeysmant, pos.z, 1.);
}