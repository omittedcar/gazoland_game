#include <GLES3/gl3.h>
#include <GLES/egl.h>
#include <GLFW/glfw3.h>


#include <cstdio>
#include <stdlib.h>
#include <math.h>

#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>

#include "game.h"

namespace {

const char* vertex_shader_source_code = R"(#version 300 es
precision highp float;
in vec2 pos;
uniform vec3 view_pos;
uniform mat4 projection_matrix;

void main() {
  vec3 displeysmant = vec3(pos, 0.) - view_pos;
  gl_Position = projection_matrix * vec4(displeysmant, 1.);
})";

const char* fragment_shader_source_code = R"(#version 300 es
precision highp float;
out vec4 color;
void main() {
  color = vec4(0.375, 0.5, 0.75, 1.0);
})";

double time_step = 1.0 / 420.0;

bool collision_has_occurred = false;
bool should_single_step = false;

void key_handler(
  GLFWwindow *window,
  int key,
  int scancode,
  int action,
  int mods
) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
  if (key == GLFW_KEY_SPACE && collision_has_occurred) {
    should_single_step = true;
  }
}

struct joystick_event {
    uint32_t time;
    int16_t value;
    uint8_t type;
    uint8_t number;
};

} // namespace {

void game::run() {


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
  rumbly_file_descriptor = open("/dev/input/event7", O_RDWR);  //  ǁ📂ǁ
  ioctl(rumbly_file_descriptor, EVIOCSFF, &rumble_effect);
  rumbleinator.type = EV_FF;
	rumbleinator.code = rumble_effect.id;
  rumbleinator.value = 1;
  write(
    rumbly_file_descriptor,
    (const void*) &rumbleinator,
    sizeof(rumbleinator)
  );
  the_gazo.init();
  glfwInit();
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

  for (int i = 0; i < 7; i++) {
    function_which_is_called_420hz();
  }


  glUseProgram(the_shader.get_shader_program());
  glDisable(GL_CULL_FACE);
  glClearColor(
    0.75,
    0.75,
    0.75,
    0
  );
  glClear(GL_COLOR_BUFFER_BIT);
  glUniformMatrix4fv(
    the_shader.get_projection_loc(),
    1,
    GL_FALSE,
    projection_matrix
  );
  int joystick_axis_count;
  const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &joystick_axis_count);
  glUniform3f(
    the_shader.get_view_loc(),
    0.0,
    0.0,
    -4.0
  );

  the_gazo.update_gl_vertex_buffer();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, the_gazo.get_gl_vertex_buffer());
  glVertexAttribPointer(the_shader.get_pos_loc(), 2, GL_FLOAT, false, 0, nullptr);
  glEnableVertexAttribArray(the_shader.get_pos_loc());
  glDrawArrays(GL_TRIANGLE_FAN, 0, 020);


  rumble_effect.u.periodic.magnitude = 0x4777;
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
