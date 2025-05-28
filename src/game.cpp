#include "game.h"
#include "./resources.h"
#include "gl_or_gles.h"
#include "path.h"

#include <GLFW/glfw3.h>
//#incude <EGL/egl.h>
//#include <cassert>
#include <stdio.h>
//#include <fcntl.h>
//#include <linux/input.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
//#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <fstream>
//#include <sstream>
#include <libgen.h>

#define RESOLUTION_X 512
#define RESOLUTION_Y 384

#define UI_WIDTH 36
#define UI_HEIGHT 20
#define UI_BYTES 0x9000
namespace {
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

GLuint load_shader_from_file(const char* path, int type) {
  GLuint result = glCreateShader(type);
  std::filesystem::path full_path(root_path());
  full_path /= "assets";
  full_path /= "glsl";
  full_path /= path;
  std::ifstream ifs(full_path.string(), std::ios::in);
  std::ostringstream oss;
  oss << ifs.rdbuf();
  std::string shader_source(oss.str());
  const char* shader_source_c = shader_source.c_str();
  glShaderSource(result, 1, &shader_source_c, nullptr);
  //CHECK_GL();
  glCompileShader(result);
  //CHECK_GL();
  GLint param;
  glGetShaderiv(result, GL_COMPILE_STATUS, &param);
  /*CHECK_GL();
  if (param != GL_TRUE) {
    std::cerr << "glCompileShader(" << full_path << ") failed." << std::endl;
  }*/
  return result;
}

void set_texture_params(int base_level, int max_level) {
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, base_level);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, max_level);
}

GLuint load_texture_from_file(const char* path) {
  GLuint result;
  std::filesystem::path full_path(root_path());
  full_path /= "assets";
  full_path /= "textures";
  full_path /= path;
  std::cout << "loading tex " << full_path.string() << std::endl;
  FILE* tex_file =fopen(full_path.c_str(), "rb");
  int width = 256;
  int height = 256;
  int buffer_size = width * height;
  void* data = malloc(buffer_size);
  //note to self: the total size of the file should be 2/3 the # of pixels
  fread(data, 1, width * height, tex_file);
  glGenTextures(1, &result);
  glBindTexture(GL_TEXTURE_2D, result);
  set_texture_params(0, 0);

    glCompressedTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,
      width,
      height,
      0,
      buffer_size,
      data
    );
  

  //CHECK_GL();
  fclose(tex_file);
  free(data);
  //glGenerateMipmap(GL_TEXTURE_2D);
  return result;
}

} // namespace


void dont_free() {
  __asm__("nop;");
};
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
  lettering = (unsigned char*) malloc(UI_BYTES + 1);
  for(int i = 0; i < UI_BYTES; i++) {
    lettering[i] = 0;
  }
  write_text(
    //"The Mechanism is a hazardous ride located in Gazoland, built over the course of five years by Gazolandic Tesseract Engineering Incorporated. It is the largest ride in Gazo Square and, like many other rides in Gazoland, carries an extreme risk of death for both those riding it and those working to maintain it. The Mechanism is only ridden by expert ride-goers as it is infamous for inflicting at least a dozen severe injuries in poorly maintained parts of the ride. It is estimated that the average ride time of The Mechanism is three days, give or take several hours, meaning riders will have to pack provisions and be prepared to make stops on ledges or at Gazolander housing complexes located sparsely throughout the body. Do not bring children to the Mechanism unless you plan to get back down when you're in the beginning of the upper parts.\n"
    "THE MECHANISM\n",
    0
  );
  
  //memcpy(lettering, font + 1, 2048);
  window = glfwCreateWindow(
    RESOLUTION_X, RESOLUTION_Y,
    "Dat, the first glaggle to ride the mechanism 2 electric boogaloo",
    nullptr, nullptr
  );
  glfwSetKeyCallback(window, key_handler);
  glfwMakeContextCurrent(window);

  
  vertshader_basic    = load_shader_from_file("vert_basic.glsl", GL_VERTEX_SHADER);
  vertshader_gazo     = load_shader_from_file("vert_gazo.glsl", GL_VERTEX_SHADER);
  vertshader_3d       = load_shader_from_file("vert_3d.glsl", GL_VERTEX_SHADER);
  vertshader_no_uv_map= load_shader_from_file("vert_no_uv_map.glsl", GL_VERTEX_SHADER);
  fragshader_basic    = load_shader_from_file("frag_basic.glsl", GL_FRAGMENT_SHADER);
  fragshader_gamma    = load_shader_from_file("frag_gamma.glsl", GL_FRAGMENT_SHADER);
  fragshader_gui      = load_shader_from_file("frag_gui.glsl", GL_FRAGMENT_SHADER);
  gazo_shader_info.link(
    vertshader_gazo, fragshader_basic,
    "view", "projection", "the_texture",
    "pos", "vert_uv"
  );
  terrain_shader_info.link(
    vertshader_3d, fragshader_basic,
    "view_pos", "projection_matrix", "the_texture",
    "pos", "vertex_uv"
  );
  polygon_fill_shader_info.link(
    vertshader_no_uv_map, fragshader_basic,
    "view_pos", "projection_matrix", "the_texture",
    "vertex_pos", nullptr
  );
  gui_shader_info.link(
    vertshader_basic, fragshader_gui,
    nullptr, nullptr, "the_ui",
    "pos", nullptr
  );

  gamma_shader_info.link(
    vertshader_basic, fragshader_gamma,
    nullptr, nullptr, nullptr, nullptr, nullptr);

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

  //glEnable(GL_DITHER);
  glBindTexture(GL_TEXTURE_2D, depth_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, RESOLUTION_X, RESOLUTION_Y, 0,
               GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
  depth_texture, 0);
  #define preffered_filter GL_LINEAR
  #define preffered_min_filter GL_LINEAR_MIPMAP_LINEAR

  glBindTexture(GL_TEXTURE_2D, gui_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, UI_WIDTH * 16, UI_HEIGHT, 0,
               GL_RED, GL_UNSIGNED_BYTE, lettering);

  the_level.construct("test_level.mechanism");
  
  
  gazo_spritesheet_texture = load_texture_from_file("aqua.pkm");
  stone_tile_texture = load_texture_from_file("grass.pkm");
  bailey_truss_texture = load_texture_from_file("aqua.pkm");
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
  glDeleteShader(fragshader_gui);
  glDeleteShader(fragshader_gamma);

  glDeleteTextures(3, &gazo_spritesheet_texture);
  glDeleteTextures(3, &framebuffer_texture);

  free(lettering);
  glfwDestroyWindow(window);
  glfwTerminate();
}

void game::the_monitor_has_refreshed_again()
{
  // if(frame_counter % 30 == 0) {
  glfwPollEvents();
  for (int i = 0; i < 6; i++) {
    function_which_is_called_480hz();
  }
  glfwGetWindowSize(window, &window_width, &window_height);
  int joystick_axis_count;
  vec2 cursor_pos;
  const float *joystick_axes = glfwGetJoystickAxes(0, &joystick_axis_count);
  glfwGetCursorPos(window, &cursor_pos.x, &cursor_pos.y);
  vec2 cursor_pos_mapped = {
    (cursor_pos.x - window_width / 2.0) * 8.0 / window_height,
    -8.0*((cursor_pos.y / window_height)-0.5)
  };
  float distance_squared = cursor_pos_mapped.x * cursor_pos_mapped.x
    + cursor_pos_mapped.y * cursor_pos_mapped.y;
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
  glDisable(GL_BLEND);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearDepthf(1.0f);
  glClear(GL_DEPTH_BUFFER_BIT);
  glUseProgram(gamma_shader_info.program);
  glBindBuffer(GL_ARRAY_BUFFER, square_buffer);
  glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, nullptr);
  glEnableVertexAttribArray(0);
  glBindTexture(GL_TEXTURE_2D, framebuffer_texture);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  glUseProgram(gui_shader_info.program);
  glBindBuffer(GL_ARRAY_BUFFER, square_buffer);
  glVertexAttribPointer(gui_shader_info.v_pos, 2, GL_FLOAT, false, 0, nullptr);
  glEnableVertexAttribArray(gui_shader_info.v_pos);
  glUniform1i(gui_shader_info.u_texture, 0);
  glBindTexture(GL_TEXTURE_2D, gui_texture);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  // rumble_effect.u.periodic.magnitude = the_gazo.get_rumble() * 0x1000;
  // ioctl(rumbly_file_descriptor, EVIOCSFF, &rumble_effect);
  // rumbleinator.code = rumble_effect.id;
  // write(rumbly_file_descriptor, (const void*) &rumbleinator,
  // sizeof(rumbleinator));
  glfwSwapBuffers(window);
  frame_counter++;
}

void game::function_which_is_called_480hz() { the_level.time_step(); }
void game::write_text(const char* text_which_we_are_writing, int offset) {
  char char_here;
  for(int i = 0; (char_here = text_which_we_are_writing[i]) != '\n'; i++) {
    memcpy(lettering + (i + (i/UI_WIDTH*UI_WIDTH)) * 16 + 1, font + char_here * 16, 16);
  }
}
