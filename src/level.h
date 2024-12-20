#include "gazo.h"
class level {
 public:
  void construct();
  void demolish();
  void time_step();
  void draw(shader* gazo_shader, GLuint gazo_texture);
 private:
  gazo the_gazo;
};