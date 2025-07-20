#include "game.h"
#include "path.h"

#include "gles_or_vulkan.h"
#include "embeds.h"
#include <math.h>
#include <string.h>
#include <iostream>

#define RESOLUTION_X 1024
#define RESOLUTION_Y 768

#define UI_WIDTH 36
#define UI_HEIGHT 20

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

std::shared_ptr<shader> load_shader_from_file(
    const char* name, shader_type type,
    const char* v_pos_name = nullptr,
    const char* v_uv_name = nullptr) {
  std::filesystem::path full_path(root_path());
  full_path /= "assets";
  full_path /= "glsl";
  full_path /= std::string(name) + ".glsl";
  return shader::create(full_path, type, v_pos_name, v_uv_name);
}

std::shared_ptr<texture> load_texture_from_file(const char* path, size_t mip_count = 0) {
  std::filesystem::path full_path(root_path());
  full_path /= "assets";
  full_path /= "textures";
  full_path /= path;
  std::cout << "loading tex " << full_path.string() << std::endl;
  return texture::create_from_file(full_path, mip_count);
}

void write_text(unsigned char* destination,
		const char* text_which_we_are_writing,
		int offset);

}  // namespace

void game::run() {
  while (state != CLEANUP) {
    update();
  }
}
void game::load() {
  lettering = (unsigned char*) malloc(k_ui_size);
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

  gazoland_init();
  
  write_text(
    //"The Mechanism is a hazardous ride located in Gazoland, built over the course of five years by Gazolandic Tesseract Engineering Incorporated. It is the largest ride in Gazo Square and, like many other rides in Gazoland, carries an extreme risk of death for both those riding it and those working to maintain it. The Mechanism is only ridden by expert ride-goers as it is infamous for inflicting at least a dozen severe injuries in poorly maintained parts of the ride. It is estimated that the average ride time of The Mechanism is three days, give or take several hours, meaning riders will have to pack provisions and be prepared to make stops on ledges or at Gazolander housing complexes located sparsely throughout the body. Do not bring children to the Mechanism unless you plan to get back down when you're in the beginning of the upper parts.\n"
    "THE MECHANISM\n",
    0
  );
  
  window = glfwCreateWindow(
    RESOLUTION_X, RESOLUTION_Y,
    "Dat, the first glaggle to ride the mechanism 2 electric boogaloo",
    nullptr, nullptr
  );
  
  glfwSetKeyCallback(window, key_handler);
  glfwMakeContextCurrent(window);

  std::shared_ptr<shader> vertshader_basic =
    load_shader_from_file("vert_basic", shader_type::k_vertex,
			  "pos", "");
  std::shared_ptr<shader> vertshader_gazo =
    load_shader_from_file("vert_gazo", shader_type::k_vertex,
			  "pos", "vert_uv");
  std::shared_ptr<shader> vertshader_3d =
    load_shader_from_file("vert_3d", shader_type::k_vertex,
			  "pos", "vertex_uv");
  std::shared_ptr<shader> vertshader_no_uv_map =
    load_shader_from_file("vert_no_uv_map", shader_type::k_vertex,
			  "vertex_pos", "");
  std::shared_ptr<shader> fragshader_basic =
    load_shader_from_file("frag_basic", shader_type::k_fragment,
			  "", "");
  std::shared_ptr<shader> fragshader_gamma =
    load_shader_from_file("frag_gamma", shader_type::k_fragment,
			  "", "");
  std::shared_ptr<shader> fragshader_gui =
    load_shader_from_file("frag_gui", shader_type::k_fragment,
			  "", "");

  gazo_prog = program::create("gazo",
      vertshader_gazo, fragshader_basic,
      "projection", "the_texture");
  terrain_prog = program::create("terrain",
      vertshader_3d, fragshader_basic,
      "projection", "the_texture");
  polygon_fill_prog = program::create("polygon_fill",
      vertshader_no_uv_map, fragshader_basic,
      "projection", "the_texture");
  gui_prog = program::create("gui",
      vertshader_basic, fragshader_gui,
      "", "the_ui");
  gamma_prog = program::create("gamma",
      vertshader_basic, fragshader_gamma,
      "", "");

  std::vector<float> the_square = 
    {-1.0, -1.0, -1.0, 1.0, 1.0, 1.0,
     -1.0, -1.0, 1.0, -1.0, 1.0, 1.0};
  square_buf = buffer::create("square", the_square, buffer_type::k_array);

  draw_fb = framebuffer::create("draw");
  draw_tex = texture::create_for_draw(RESOLUTION_X, RESOLUTION_Y, draw_fb);
  depth_tex = texture::create_for_depth(RESOLUTION_X, RESOLUTION_Y, draw_fb);
  gui_tex = texture::create_for_gui(UI_WIDTH, UI_HEIGHT, lettering);

  gazo_spritesheet_tex = load_texture_from_file("aqua.pkm");
  stone_tile_tex = load_texture_from_file("grass.pkm");
  bailey_truss_tex = load_texture_from_file("aqua.pkm");

  the_gazo = std::make_shared<gazo>(gazo_prog, gazo_spritesheet_tex);
  the_level = std::make_unique<level>(
      "test_level.mechanism", the_gazo, terrain_prog, polygon_fill_prog,
      stone_tile_tex);
}
void game::update() {
  if(state == LOADING) {
    load();
    state = PLAYING;
  } else if((state = glfwWindowShouldClose(window) ? CLEANUP : state) == PLAYING) {
    the_monitor_has_refreshed_again();
  } else if(state == CLEANUP) {
    unload();
  }
}
void game::unload() {
  free(lettering);
}
game::~game() {
  gazoland_cleanup();
  glfwDestroyWindow(window);
  glfwTerminate();
}

void game::the_monitor_has_refreshed_again()
{
  // if(frame_counter % 30 == 0) {
  glfwPollEvents();
  for (int i = 0; i < 8; i++) {
    function_which_is_called_480hz();
  }
  int window_width, window_height;
  glfwGetWindowSize(window, &window_width, &window_height);
  float xscale, yscale;
  glfwGetWindowContentScale(window, &xscale, &yscale);

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
    the_level->control_gazo(joystick_axes[0],-joystick_axes[1],
                           joystick_axes[5], -joystick_axes[2]);
  } else {
    the_level->control_gazo(
      cursor_pos_mapped.x, cursor_pos_mapped.y,
      cursor_pos_mapped.x, cursor_pos_mapped.y
    );
  }

  fvec2 view = the_gazo->get_center_of_mass_medium_precision();
  float aspect = 4.0/3.0;
  float view_area = 64.0;
  std::vector<float> projection_matrix{
      2 * sqrt(1/aspect / view_area), 0, 0, 0,
      0, 2 * sqrt(aspect / view_area), 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1};
  
  prepare_to_draw(draw_fb, RESOLUTION_X, RESOLUTION_Y);
  the_gazo->draw(projection_matrix, view);
  the_level->draw(projection_matrix, view);
  
  present_game(window_width * xscale, window_height * yscale,
               gamma_prog, square_buf, draw_tex);
  present_gui(gui_prog, square_buf, gui_tex);
  
  // rumble_effect.u.periodic.magnitude = the_gazo.get_rumble() * 0x1000;
  // ioctl(rumbly_file_descriptor, EVIOCSFF, &rumble_effect);
  // rumbleinator.code = rumble_effect.id;
  // write(rumbly_file_descriptor, (const void*) &rumbleinator,
  // sizeof(rumbleinator));
  glfwSwapBuffers(window);
  frame_counter++;
}

void game::function_which_is_called_480hz() { the_level->time_step(); }
void game::write_text(
		const char* text_which_we_are_writing,
		int offset) {
  char char_here;
  for(int i = 0; (char_here = text_which_we_are_writing[i]) != '\n'; i++) {
    memcpy(lettering + (i + (i/UI_WIDTH*UI_WIDTH)) * 16 + 1, font + char_here * 16, 16);
  }
}
