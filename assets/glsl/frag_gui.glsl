#version 310 es

precision highp float;
out vec4 color;
in vec2 uv;
uniform sampler2D the_ui;
void main() {
  ivec2 the_coord = ivec2(
    (uv * vec2(1.0,-1.0) + vec2(0.0,1.0))
    * vec2(textureSize(the_ui,0).xy) * vec2(0.5,8.0)
  );
  int the_number = int(
    texelFetch(
      the_ui,
      ivec2(
        (the_coord.x << 1) + 2 - ((the_coord.y >> 2) & 1),
        the_coord.y >> 3
      ),
      0
    ).x * 255.0
  );
  int y_mod_4 = the_coord.y & 3;
  int the_other_number = (the_number >> (y_mod_4 * 2)) % 4;
  if(the_other_number == 0) {
    discard;
  }
  color = vec4(
    sqrt(float(the_other_number) / 2.0 - 0.5),
    sqrt(float(the_other_number) / 2.0 - 0.5),
    sqrt(float(the_other_number) / 2.0 - 0.5),
    1.0
  );
}