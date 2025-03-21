#include "game.h"
#include "./resources.h"
#include "gl_or_gles.h"
#include "png_decoder.h"

#include <GLFW/glfw3.h>
//#incude <EGL/egl.h>
#include <cassert>
#include <cstdio>
#include <fcntl.h>
#include <linux/input.h>
#include <math.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define RESOLUTION_X 1080
#define RESOLUTION_Y 720

#define UI_RESOLUTION_X 288
#define UI_RESOLUTION_Y 192
#define UI_AREA_DI_VIDED_BY_8 6912

namespace
{
  double time_step = 1.0 / 480.0;

  bool collision_has_occurred = false;
  bool should_single_step = false;

  void key_handler(GLFWwindow *window, int key, int scancode, int action,
                   int mods)
  {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
      printf(
          "bro you just preessed the space bar bgro that doesn't do anything \n");
    }
  }

  struct joystick_event
  {
    uint32_t time;
    int16_t value;
    uint8_t type;
    uint8_t number;
  };

  bool maybe_print_error(const char *file, size_t line)
  {
    GLenum err;
    bool result = false;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
      result = true;
      printf("GL error code 0x%x prior to %s:%zu\n", err, file, line);
    }
    return result;
  }

#define CHECK_GL() maybe_print_error(__FILE__, __LINE__)

} // namespace

void dump(uint8_t *data, int size)
{
  for (int i = 0; i < size; i++)
  {
    if (i % 8 == 0)
    {
      printf("\n");
    }
    printf("%02X ", data[i]);
  }
  printf("\n");
}

void game::run()
{
  info_log = (char*) malloc(69420);
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
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  //glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  
  window = glfwCreateWindow(
    RESOLUTION_X, RESOLUTION_Y,
    "Dat, the first glaggle to ride the mechanism 2 electric boogaloo",
    nullptr, nullptr
  );
  glfwSetKeyCallback(window, key_handler);
  glfwMakeContextCurrent(window);
  vertshader_basic = glCreateShader(GL_VERTEX_SHADER);
  vertshader_gazo = glCreateShader(GL_VERTEX_SHADER);
  vertshader_3d = glCreateShader(GL_VERTEX_SHADER);
  vertshader_no_uv_map = glCreateShader(GL_VERTEX_SHADER);
  fragshader_basic = glCreateShader(GL_FRAGMENT_SHADER);
  fragshader_gamma = glCreateShader(GL_FRAGMENT_SHADER);
  fragshader_gui = glCreateShader(GL_FRAGMENT_SHADER);
  {
    void *some_pointers[] = {(void *)vert_basic_glsl, nullptr};
    glShaderSource(vertshader_basic, 1, (const char *const *)(some_pointers),
                   nullptr);
    *some_pointers = (void *)vert_gazo_glsl;
    glShaderSource(vertshader_gazo, 1, (const char *const *)(some_pointers),
                   nullptr);
    *some_pointers = (void *)vert_3d_glsl;
    glShaderSource(vertshader_3d, 1, (const char *const *)some_pointers,
                   nullptr);
    *some_pointers = (void *)vert_no_uv_map_glsl;
    glShaderSource(vertshader_no_uv_map, 1, (const char *const *)some_pointers,
                   nullptr);
    *some_pointers = (void *)frag_basic_glsl;
    glShaderSource(fragshader_basic, 1, (const char *const *)some_pointers,
                   nullptr);
    *some_pointers = (void *)frag_gamma_glsl;
    glShaderSource(fragshader_gamma, 1, (const char *const *)some_pointers,
                   nullptr);
    *some_pointers = (void *)frag_gui_glsl;
    glShaderSource(fragshader_gui, 1, (const char *const *)some_pointers,
                   nullptr);
  };
  CHECK_GL();
  glCompileShader(vertshader_basic);
  CHECK_GL();
  glCompileShader(vertshader_gazo);
  CHECK_GL();
  glCompileShader(vertshader_no_uv_map);
  glCompileShader(vertshader_3d);
  CHECK_GL();
  glCompileShader(fragshader_basic);
  CHECK_GL();
  glCompileShader(fragshader_gamma);
  CHECK_GL();
  gazo_shader_info.shader = glCreateProgram();
  terrain_shader_info.shader = glCreateProgram();
  polygon_fill_shader_info.shader = glCreateProgram();
  gamma_shader = glCreateProgram();
  CHECK_GL();
  glAttachShader(gazo_shader_info.shader, vertshader_gazo);
  glAttachShader(gazo_shader_info.shader, fragshader_basic);
  glAttachShader(terrain_shader_info.shader, vertshader_3d);
  glAttachShader(terrain_shader_info.shader, fragshader_basic);
  glAttachShader(polygon_fill_shader_info.shader, vertshader_no_uv_map);
  glAttachShader(polygon_fill_shader_info.shader, fragshader_basic);
  glAttachShader(gamma_shader, vertshader_basic);
  glAttachShader(gamma_shader, fragshader_gamma);
  glLinkProgram(gazo_shader_info.shader);
  CHECK_GL();

  gazo_shader_info.u_panning = glGetUniformLocation(gazo_shader_info.shader, "view");
  gazo_shader_info.u_projection = glGetUniformLocation(gazo_shader_info.shader, "projection");
  gazo_shader_info.u_texture = glGetUniformLocation(gazo_shader_info.shader, "the_texture");
  gazo_shader_info.v_pos = glGetAttribLocation(gazo_shader_info.shader, "pos");
  gazo_shader_info.v_uv = glGetAttribLocation(gazo_shader_info.shader, "vert_uv");

  
  glLinkProgram(terrain_shader_info.shader);
  
  terrain_shader_info.u_panning = glGetUniformLocation(terrain_shader_info.shader, "view_pos");
  terrain_shader_info.u_projection = glGetUniformLocation(terrain_shader_info.shader, "projection_matrix");
  terrain_shader_info.u_texture = glGetUniformLocation(terrain_shader_info.shader, "the_texture");
  terrain_shader_info.v_pos = glGetAttribLocation(terrain_shader_info.shader, "pos");
  terrain_shader_info.v_uv = glGetAttribLocation(terrain_shader_info.shader, "vertex_uv");

  glLinkProgram(polygon_fill_shader_info.shader);
  CHECK_GL();
  polygon_fill_shader_info.u_panning = glGetUniformLocation(polygon_fill_shader_info.shader, "view_pos");
  polygon_fill_shader_info.u_projection = glGetUniformLocation(polygon_fill_shader_info.shader, "projection_matrix");
  polygon_fill_shader_info.u_texture = glGetUniformLocation(polygon_fill_shader_info.shader, "the_texture");
  polygon_fill_shader_info.v_pos = glGetAttribLocation(polygon_fill_shader_info.shader, "vertex_pos");

  glLinkProgram(gamma_shader);
  CHECK_GL();

  {
    float the_square[] = {-1.0, -1.0, -1.0, 1.0, 1.0, 1.0,
                          -1.0, -1.0, 1.0, -1.0, 1.0, 1.0};
    glGenBuffers(1, &square_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, square_buffer);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), the_square,
                 GL_STATIC_DRAW);
  };

  glGenFramebuffers(1, &framebuffer);
  glGenTextures(3, &framebuffer_texture);
  glBindTexture(GL_TEXTURE_2D, framebuffer_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, RESOLUTION_X, RESOLUTION_Y, 0, GL_RGB,GL_UNSIGNED_INT_10F_11F_11F_REV, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         framebuffer_texture, 0);

  glEnable(GL_DITHER);
  glBindTexture(GL_TEXTURE_2D, depth_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, RESOLUTION_X, RESOLUTION_Y, 0,
               GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
  depth_texture, 0);

  glBindTexture(GL_TEXTURE_2D, gui_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, 2, UI_AREA_DI_VIDED_BY_8, 0,
               GL_RED, GL_BYTE, nullptr);

  the_level.construct();
  glGenTextures(3, &gazo_spritesheet_texture);

  #define preffered_filter GL_LINEAR
  #define preffered_min_filter GL_LINEAR_MIPMAP_NEAREST
  glBindTexture(GL_TEXTURE_2D, gazo_spritesheet_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, preffered_min_filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, preffered_filter);
  decode_png_truecolor(gazo_spritesheet_png, gazo_spritesheet_png_len);
  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, stone_tile_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, preffered_min_filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, preffered_filter);
  decode_png_truecolor(stone_tile_png, stone_tile_png_len);
  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, bailey_truss_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, preffered_min_filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, preffered_filter);
  decode_png_truecolor(bailey_truss_png, bailey_truss_png_len);
  glGenerateMipmap(GL_TEXTURE_2D);
  while (is_playing && !glfwWindowShouldClose(window))
  {
    the_monitor_has_refreshed_again();
  }
}

void game::stop()
{
  the_level.demolish();
  glDeleteFramebuffers(1, &framebuffer);
  glDeleteBuffers(1, &square_buffer);
  glDeleteShader(vertshader_basic);
  glDeleteShader(vertshader_gazo);
  glDeleteShader(vertshader_3d);
  glDeleteShader(vertshader_no_uv_map);
  glDeleteShader(fragshader_basic);
  glDeleteShader(fragshader_gamma);
  glDeleteShader(gazo_shader_info.shader);
  glDeleteShader(terrain_shader_info.shader);
  glDeleteShader(polygon_fill_shader_info.shader);
  glDeleteShader(gamma_shader);
  glDeleteTextures(3, &gazo_spritesheet_texture);
  glDeleteTextures(3, &framebuffer_texture);
  glfwDestroyWindow(window);
  glfwTerminate();
}

void game::the_monitor_has_refreshed_again()
{
  // if(frame_counter % 30 == 0) {
  glfwGetWindowSize(window, &window_width, &window_height);
  int joystick_axis_count;
  vec2 cursor_pos;
  const float *joystick_axes = glfwGetJoystickAxes(0, &joystick_axis_count);
  glfwGetCursorPos(window, &cursor_pos.x, &cursor_pos.y);
  vec2 cursor_pos_mapped = {(cursor_pos.x - window_width / 2.0) * 8.0 / window_height, -8.0*((cursor_pos.y / window_height)-0.5)};
  float distance_squared = cursor_pos_mapped.x * cursor_pos_mapped.x + cursor_pos_mapped.y * cursor_pos_mapped.y;
  if(distance_squared > 1.0) {
    double inverse_distance = 1 / sqrt(distance_squared); // theres a machine instruction >:(
    cursor_pos_mapped.x *= inverse_distance;
    cursor_pos_mapped.y *= inverse_distance;
  }
  if (joystick_axis_count >= 6)
  {
    the_level.control_gazo(joystick_axes[0],-joystick_axes[1],
                           joystick_axes[5], -joystick_axes[2]);
  } else {
    the_level.control_gazo(
      cursor_pos_mapped.x, cursor_pos_mapped.y,
      cursor_pos_mapped.x, cursor_pos_mapped.y
    );
  }


  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glViewport(0, 0, RESOLUTION_X, RESOLUTION_Y);
  the_level.draw(
    &gazo_shader_info, &terrain_shader_info, &polygon_fill_shader_info,
    gazo_spritesheet_texture, stone_tile_texture
  );
  {
    glViewport(0, 0, window_width, window_height);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearDepthf(1.0f);
  glClear(GL_DEPTH_BUFFER_BIT);
  glUseProgram(gamma_shader);
  glBindBuffer(GL_ARRAY_BUFFER, square_buffer);
  glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, nullptr);
  glEnableVertexAttribArray(0);
  glBindTexture(GL_TEXTURE_2D, framebuffer_texture);
  
  glDrawArrays(GL_TRIANGLES, 0, 6);

  // rumble_effect.u.periodic.magnitude = the_gazo.get_rumble() * 0x1000;
  // ioctl(rumbly_file_descriptor, EVIOCSFF, &rumble_effect);
  // rumbleinator.code = rumble_effect.id;
  // write(rumbly_file_descriptor, (const void*) &rumbleinator,
  // sizeof(rumbleinator));
    for (int i = 0; i < 8; i++)
  {
    function_which_is_called_480hz();
  }
  glfwSwapBuffers(window);
  glfwPollEvents();
  frame_counter++;
}

void game::function_which_is_called_480hz() { the_level.time_step(); }
