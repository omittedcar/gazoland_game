#version 300 es

precision highp float;
in vec2 uv;
out vec4 color;
uniform sampler2D the_texture;
float gamma(float x) {
  return  
    (1.15907984 * inversesqrt(x + 0.00279491) - 0.15746346551006618) * x;
}

void main() {
  vec3 color_linear = texelFetch(the_texture,ivec2(gl_FragCoord.xy),0).xyz;
  color = vec4(
    gamma(color_linear.x),
    gamma(color_linear.y),
    gamma(color_linear.z),
    1.0
  );
} 