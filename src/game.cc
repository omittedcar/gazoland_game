#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdio>
#include <stdlib.h>
#include <math.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include "gl_or_gles.h"
#include "game.h"
#include "./resources.h"
#include "png_decoder.h"

#

namespace {

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
  if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
    printf("bro you just preessed the space bar bgro that doesn't do anything \n");
  }
}

struct joystick_event {
    uint32_t time;
    int16_t value;
    uint8_t type;
    uint8_t number;
};

void maybe_print_error(size_t line) {
  GLenum err;
  while((err = glGetError()) != GL_NO_ERROR) {
    printf("GL error code 0x%x prior to line %zu\n", err, line);
  }
}
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

  is_playing = true;

  /* 
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
  */
  
  glfwInit();
  glfwSwapInterval(1);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_SAMPLES, 0);

  window = glfwCreateWindow(
    0x300,
    0x200,
    "dorito",
    nullptr,
    nullptr
  );
  glfwSetKeyCallback(window, key_handler);
  glfwMakeContextCurrent(window);
  vertshader_basic = glCreateShader(GL_VERTEX_SHADER);
  maybe_print_error(__LINE__);
  vertshader_gazo = glCreateShader(GL_VERTEX_SHADER);
  maybe_print_error(__LINE__);
  vertshader_3d = glCreateShader(GL_VERTEX_SHADER);
  maybe_print_error(__LINE__);
  fragshader_basic = glCreateShader(GL_FRAGMENT_SHADER);
  maybe_print_error(__LINE__);
  fragshader_gamma = glCreateShader(GL_FRAGMENT_SHADER);
  maybe_print_error(__LINE__);
  printf("bouta shader source\n");
  {
    void* some_pointers[] = {
      (void*) vert_basic_glsl,
      nullptr
    };
    glShaderSource(
      vertshader_basic,
      1,
      (const char* const *) (some_pointers),
      nullptr
    );
    maybe_print_error(__LINE__);
    some_pointers[0] = (void*) vert_gazo_glsl;
    glShaderSource(
      vertshader_gazo,
      1,
      (const char* const *) (some_pointers),
      nullptr
    );
    maybe_print_error(__LINE__);
    some_pointers[0] = (void*) vert_3d_glsl;
    glShaderSource(
      vertshader_3d,
      1,
      (const char* const*) some_pointers,
      nullptr
    );
    maybe_print_error(__LINE__);
    some_pointers[0] = (void*) frag_basic_glsl;
    glShaderSource(
      fragshader_basic,
      1,
      (const char* const*) some_pointers,
      nullptr
    );
    maybe_print_error(__LINE__);
    some_pointers[0] = (void*) frag_gamma_glsl;
    glShaderSource(
      fragshader_gamma,
      1,
      (const char* const*) some_pointers,
      nullptr
    );
    maybe_print_error(__LINE__);
  };
  printf("finna shader source\n");
  glCompileShader(vertshader_basic);
  maybe_print_error(__LINE__);
  glCompileShader(vertshader_gazo);
  maybe_print_error(__LINE__);
  glCompileShader(vertshader_3d);
  maybe_print_error(__LINE__);
  glCompileShader(fragshader_basic);
  maybe_print_error(__LINE__);
  glCompileShader(fragshader_gamma);
  maybe_print_error(__LINE__);
  gazo_shader = glCreateProgram();
  maybe_print_error(__LINE__);
  terrain_shader = glCreateProgram();
  maybe_print_error(__LINE__);
  gamma_shader = glCreateProgram();
  maybe_print_error(__LINE__);
  glAttachShader(gazo_shader, vertshader_gazo);
  maybe_print_error(__LINE__);
  glAttachShader(gazo_shader, fragshader_basic);
  maybe_print_error(__LINE__);
  glAttachShader(terrain_shader, vertshader_3d);
  maybe_print_error(__LINE__);
  glAttachShader(terrain_shader, fragshader_basic);
  maybe_print_error(__LINE__);
  glAttachShader(gamma_shader, vertshader_basic);
  maybe_print_error(__LINE__);
  glAttachShader(gamma_shader, fragshader_gamma);
  maybe_print_error(__LINE__);
  glLinkProgram(gazo_shader);
  maybe_print_error(__LINE__);
  glLinkProgram(terrain_shader);
  maybe_print_error(__LINE__);
  glLinkProgram(gamma_shader);
  maybe_print_error(__LINE__);

  {
    float the_square[] = {
      -1.0, -1.0,
      -1.0, 1.0,
      1.0, 1.0,
      -1.0, -1.0,
      1.0, -1.0,
      1.0, 1.0
    };
    glGenBuffers(1, &square_buffer);
  maybe_print_error(__LINE__);
    glBindBuffer(GL_ARRAY_BUFFER, square_buffer);
  maybe_print_error(__LINE__);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), the_square, GL_STATIC_DRAW);
  maybe_print_error(__LINE__);
  };

  glGenFramebuffers(1, &framebuffer);
  maybe_print_error(__LINE__);
  glGenTextures(1, &framebuffer_texture);
  maybe_print_error(__LINE__);
  glBindTexture(GL_TEXTURE_2D, framebuffer_texture);
  maybe_print_error(__LINE__);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0x240, 0x180, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
  maybe_print_error(__LINE__);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  maybe_print_error(__LINE__);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  maybe_print_error(__LINE__);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  maybe_print_error(__LINE__);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer_texture, 0);
  maybe_print_error(__LINE__);

  glGenTextures(1, &depth_texture);
  maybe_print_error(__LINE__);
  glBindTexture(GL_TEXTURE_2D, depth_texture);
  maybe_print_error(__LINE__);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 0x240, 0x180, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
  maybe_print_error(__LINE__);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, framebuffer_texture, 0);

  the_level.construct();
  glGenTextures(1, &gazo_spritesheet_texture);
  maybe_print_error(__LINE__);
  glGenTextures(1, &stone_tile_texture);
  maybe_print_error(__LINE__);

  glBindTexture(GL_TEXTURE_2D, gazo_spritesheet_texture);
  maybe_print_error(__LINE__);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  maybe_print_error(__LINE__);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  maybe_print_error(__LINE__);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  maybe_print_error(__LINE__);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  maybe_print_error(__LINE__);
  decode_png_truecolor(gazo_spritesheet_png,gazo_spritesheet_png_len);
  
  glBindTexture(GL_TEXTURE_2D, stone_tile_texture);
  maybe_print_error(__LINE__);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  maybe_print_error(__LINE__);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  maybe_print_error(__LINE__);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  maybe_print_error(__LINE__);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  maybe_print_error(__LINE__);
  decode_png_truecolor(stone_tile_png,stone_tile_png_len);
  

  while (is_playing && !glfwWindowShouldClose(window)) {
    the_monitor_has_refreshed_again();
    
  }

}

void game::stop() {
  the_level.demolish();
  glDeleteFramebuffers(1, &framebuffer);
  maybe_print_error(__LINE__);
  glDeleteBuffers(1, &square_buffer);
  maybe_print_error(__LINE__);
  glDeleteShader(vertshader_basic);
  maybe_print_error(__LINE__);
  glDeleteShader(vertshader_gazo);
  maybe_print_error(__LINE__);
  glDeleteShader(vertshader_3d);
  maybe_print_error(__LINE__);
  glDeleteShader(fragshader_basic);
  maybe_print_error(__LINE__);
  glDeleteShader(fragshader_gamma);
  maybe_print_error(__LINE__);
  glDeleteShader(gazo_shader);
  maybe_print_error(__LINE__);
  glDeleteShader(terrain_shader);
  maybe_print_error(__LINE__);
  glDeleteShader(gamma_shader);
  maybe_print_error(__LINE__);
  glDeleteTextures(1, &gazo_spritesheet_texture);
  maybe_print_error(__LINE__);
  glDeleteTextures(1, &stone_tile_texture);
  maybe_print_error(__LINE__);
  glDeleteTextures(1, &framebuffer_texture);
  maybe_print_error(__LINE__);
  glDeleteTextures(1, &depth_texture);
  maybe_print_error(__LINE__);
  glfwDestroyWindow(window);
  glfwTerminate();
}

void game::the_monitor_has_refreshed_again() {
  //if(frame_counter % 30 == 0) {

  int joystick_axis_count;
  const float* joystick_axes = glfwGetJoystickAxes(0, &joystick_axis_count);
  if(joystick_axis_count >= 6) {
    the_level.control_gazo(
      joystick_axes[0],
      -joystick_axes[1],
      joystick_axes[5],
      -joystick_axes[2]
    );
  }
  
  for (int i = 0; i < 8; i++) {
   function_which_is_called_480hz();
  }
  glDisable(GL_DEPTH_TEST);
  maybe_print_error(__LINE__);
  //glDepthFunc(GL_LEQUAL);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  maybe_print_error(__LINE__);
  glViewport(0, 0, 0x240, 0x180);
  maybe_print_error(__LINE__);
  the_level.draw(gazo_shader,terrain_shader,gazo_spritesheet_texture,stone_tile_texture);
  {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    printf("width=%d height=%d\n", width, height);
    glViewport(0, 0, width, height);
  maybe_print_error(__LINE__);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  maybe_print_error(__LINE__);
  glUseProgram(gamma_shader);
  maybe_print_error(__LINE__);
  glBindBuffer(GL_ARRAY_BUFFER, square_buffer);
  maybe_print_error(__LINE__);
  glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, nullptr);
  maybe_print_error(__LINE__);
  glEnableVertexAttribArray(0);
  maybe_print_error(__LINE__);
  glBindTexture(GL_TEXTURE_2D, framebuffer_texture);
  maybe_print_error(__LINE__);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  maybe_print_error(__LINE__);

  //rumble_effect.u.periodic.magnitude = the_gazo.get_rumble() * 0x1000;
  //ioctl(rumbly_file_descriptor, EVIOCSFF, &rumble_effect);
  //rumbleinator.code = rumble_effect.id;
  //write(rumbly_file_descriptor, (const void*) &rumbleinator, sizeof(rumbleinator));
  glfwSwapBuffers(window);
  glfwPollEvents();
  frame_counter++;
}

void game::function_which_is_called_480hz() {
  the_level.time_step();
}
