
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>

class shader {
 public:
  void init(const char* vertex_shader_source_code, const char* fragment_shader_source_code);
  GLuint get_pos_loc();
  GLuint get_uv_loc();
  GLuint get_projection_loc();
  GLuint get_view_loc();
  GLuint get_texture_loc();
  GLuint get_shader_program();
  void put_away();
 private:
  char* info_log;
  GLuint vertex_shader;
  GLuint fragment_shader;
  GLuint shader_program;
  GLuint pos_loc;
  GLuint projection_loc;
  GLuint view_loc;
  GLuint uv_loc;
  GLuint texture_loc;
};
