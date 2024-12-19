#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>


#include <cstdio>
#include <stdlib.h>
#include <math.h>

#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>


#include "game.h"
#include "./resources.h"
#include "png_decoder.h"

//const char sample_text[] = {
//#embed "./sample_text.txt"
//};

namespace {

const char* vertex_shader_source_code = R"(#version 300 es
precision highp float;
in vec2 pos;
in vec2 vertex_uv;
uniform vec3 view_pos;
uniform mat4 projection_matrix;
out vec2 uv;
void main() {
  uv = vertex_uv;
  vec3 displeysmant = vec3(pos, 0.) - view_pos;
  gl_Position = projection_matrix * vec4(displeysmant, 1.);
})";

const char* fragment_shader_source_code = R"(#version 300 es

precision highp float;
in vec2 uv;
out vec4 color;
uniform sampler2D the_texture;

void main() {

  ivec4 pallete[8] = ivec4[](
  //  R    G    B    A
    ivec4(0x00,0x00,0x00,0x00), //transparent background
    ivec4(0x00,0x00,0x00,0xFF), //dark outline + eyes
    ivec4(0x1A,0x02,0x14,0xFF), //mouth
    ivec4(0x39,0x1F,0x5A,0xFF), //transition from mouth
    ivec4(0x19,0x19,0x5E,0xFF), //transition from darkness
    ivec4(0x35,0x35,0x7F,0xFF), //more transition from darkness
    ivec4(0x41,0x41,0x98,0xFF), //base color
    ivec4(0x4A,0x4A,0xA2,0xFF)  //light gradient
  );

  
  vec2 pixel_coord = uv * vec2(textureSize(the_texture, 0));
  int bit_offset = (int(pixel_coord.x * 2.0) % 2)*3+2;
  int multipixel_byte = int(
    texelFetch(the_texture, ivec2(pixel_coord),0)
    * 255.0
  );
  int color_index = (multipixel_byte >> bit_offset) % 8;
  color = vec4(pallete[color_index]) / 255.0;
  
  //color = vec4(texture(the_texture,uv).x,0.0,0.0,1.0);
})";

double time_step = 1.0 / 420.0;

bool collision_has_occurred = false;
bool should_single_step = false;

void key_handler(
    GLFWwindow *window,
    int key,
    int scancode,
    int action,
    int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
}

struct joystick_event {
    uint32_t time;
    int16_t value;
    uint8_t type;
    uint8_t number;
};

} // namespace {

void dump(uint8_t* data, int size) {
  for(int i = 0; i < size; i++) {
    if(i % 8 == 0) {
      printf("\n");
    }
    printf("%02X ", data[i]);
  }
  printf("\n");
} 

void game::run() {

  printf("%s\n",gazo_spritesheet_png);

  is_playing = true;

  joystick_file_descriptor = open("/dev/input/js0", O_RDONLY);

  rumble_effect.type = FF_PERIODIC;
  rumble_effect.id = -1;
  rumble_effect.u.periodic.waveform = FF_SQUARE;
  rumble_effect.u.periodic.period = 0x100;
	rumble_effect.u.periodic.magnitude = 0x7fff;
	rumble_effect.u.periodic.offset = 0;
	rumble_effect.u.periodic.phase = 0;
	rumble_effect.direction = 0x4000;
	rumble_effect.u.periodic.envelope.attack_length = 0x000;
	rumble_effect.u.periodic.envelope.fade_length = 0x80;
  rumble_effect.u.periodic.envelope.attack_level = 0x0000;
	rumble_effect.u.periodic.envelope.fade_level = 0x0000;
	rumble_effect.trigger.button = 0;
	rumble_effect.trigger.interval = 0;
	rumble_effect.replay.length = 0x100;
	rumble_effect.replay.delay = 0;
  rumbly_file_descriptor = open("/dev/input/event24", O_RDWR);  //  ǁ📂ǁ
  ioctl(rumbly_file_descriptor, EVIOCSFF, &rumble_effect);
  rumbleinator.type = EV_FF;
	rumbleinator.code = rumble_effect.id;
  rumbleinator.value = 1;
  write(
    rumbly_file_descriptor,
    (const void*) &rumbleinator,
    sizeof(rumbleinator)
  );

  
  glfwInit();
  glfwSwapInterval(1);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  

  window = glfwCreateWindow(
    0x300,
    0x200,
    "dorito",
    nullptr,
    nullptr
  );
  glfwSetKeyCallback(window, key_handler);
  glfwMakeContextCurrent(window);
  the_shader.init(
    vertex_shader_source_code,
    fragment_shader_source_code
  );
  the_gazo.init();
  glGenTextures(1, &gazo_spritesheet_texture);
  glBindTexture(GL_TEXTURE_2D, gazo_spritesheet_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  decode_png(gazo_spritesheet_png,gazo_spritesheet_png_len);

  while (is_playing && !glfwWindowShouldClose(window)) {
    the_monitor_has_refreshed_again();
  }
}

void game::stop() {
  the_gazo.kill_to_death();
  the_shader.put_away();
  glfwDestroyWindow(window);
  glfwTerminate();
}

void game::the_monitor_has_refreshed_again() {
  //if(frame_counter % 30 == 0) {
  int joystick_axis_count;
  const float* joystick_axes = glfwGetJoystickAxes(0, &joystick_axis_count);
  the_gazo.point_joystick(joystick_axes[0], -joystick_axes[1]);
  the_gazo.point_other_joystick(joystick_axes[5], -joystick_axes[2]);
  for (int i = 0; i < 7; i++) {
    function_which_is_called_420hz();
  }

  glClearColor(
    0.75,
    0.75,
    0.75,
    0
  );
  glClear(GL_COLOR_BUFFER_BIT);

  the_gazo.render(the_shader,projection_matrix,view,gazo_spritesheet_texture);

  rumble_effect.u.periodic.magnitude = the_gazo.get_rumble() * 0x1000;
  ioctl(rumbly_file_descriptor, EVIOCSFF, &rumble_effect);
  rumbleinator.code = rumble_effect.id;
  write(rumbly_file_descriptor, (const void*) &rumbleinator, sizeof(rumbleinator));
  glfwSwapBuffers(window);
  glfwPollEvents();
  frame_counter++;
}

void game::function_which_is_called_420hz() {
  the_gazo.advance_forward(time_step);
}
