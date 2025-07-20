#version 310 es

precision highp float;
in vec2 uv;
out vec4 color;
uniform sampler2D the_texture;

void main() {
  vec4 texel = texture(the_texture,uv);
  //color = vec4(texel.xyz * (round(texel.w) / max(texel.w, 0.5)), round(texel.w));
  color = texel;
}