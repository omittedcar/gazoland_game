#version 310 es

precision highp float;
layout(location = 0) in vec2 pos;
layout(location = 0) out vec2 uv;
void main() {
  uv = (pos+vec2(1.0)) / 2.0;
  gl_Position = vec4(pos,0.0,1.0);
}