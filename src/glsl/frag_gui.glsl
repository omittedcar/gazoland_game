#version 300 es

precision highp float;
in vec2 uv;
out vec4 color;
uniform sampler2D the_ui;

void main() {
  
  color = texture(the_texture,uv);
} 