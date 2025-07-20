#version 310 es

precision highp float;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
layout(binding = 0) uniform sampler2D the_texture;

void main() {
  vec4 texel = texture(the_texture,uv);
  //color = vec4(texel.xyz * (round(texel.w) / max(texel.w, 0.5)), round(texel.w));
  color = texel;
}