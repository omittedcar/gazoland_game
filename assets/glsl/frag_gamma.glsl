#version 310 es

precision highp float;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
layout(binding = 0) uniform sampler2D the_texture;
float gamma(float x) {
  return  
    (1.15907984 * inversesqrt(x + 0.00279491) - 0.15746346551006618) * x;
    //float(floatBitsToInt(x)) * .0000000005;
}

void main() {
  vec3 color_linear = texelFetch(the_texture,ivec2(gl_FragCoord.xy),0).xyz;
  //vec3 color_tonemapped = color_linear * 2.0 / (color_linear * 2.0 + vec3(1.0));
  color = vec4(
    gamma(color_linear.x),
    gamma(color_linear.y),
    gamma(color_linear.z),
    //color_linear,
    1.0
  );
}