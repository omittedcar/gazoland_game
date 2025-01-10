#include "platform.h"
#include <GLES3/gl3.h>
#include <stdlib.h>

void platform::arise() {
  side_count = 8;
  corners = (float*) malloc(side_count * 2 * sizeof(float));
  corners[0] = -1.0;
  corners[1] = -1.0; 
  corners[2] = -0.7;
  corners[3] = -1.7;
  corners[4] = 0.0;
  corners[5] = -2.0;
  corners[6] = 0.7;
  corners[7] = -1.7;
  corners[8] = 1.0;
  corners[9] = -1.0;
  corners[10] = 0.7;
  corners[11] = -0.3;
  corners[12] = 0.0;
  corners[13] = 0.0;
  corners[14] = -0.7;
  corners[15] = -0.3;
  compute_bounding_box();
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, side_count * 2 * sizeof(float), corners, GL_STATIC_DRAW);
}

void platform::demolish() {
  free(corners);
  glDeleteBuffers(1, &vertex_buffer);
}

void platform::draw(
  shader* rendering_shader,
  float projection_matrix[20],
  float view[3],
  GLuint texture
) {
  glUseProgram((*rendering_shader).get_shader_program());
  glUniformMatrix4fv(
    rendering_shader->get_projection_loc(),
    1,
    GL_FALSE,
    projection_matrix
  );

  glUniform3f(
    rendering_shader->get_view_loc(),
    view[0],
    view[1],
    view[2]
  );

  glUniform1i(
    rendering_shader->get_texture_loc(),
    0
  );
  glBindTexture(GL_TEXTURE_2D, texture);
  

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glVertexAttribPointer(rendering_shader->get_pos_loc(), 2, GL_FLOAT, false, 0, nullptr);
  glEnableVertexAttribArray(rendering_shader->get_pos_loc());

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glVertexAttribPointer(rendering_shader->get_uv_loc(), 2, GL_FLOAT, false, 0, nullptr);
  glEnableVertexAttribArray(rendering_shader->get_uv_loc());
  glLineWidth(32.0);
  glDrawArrays(GL_LINE_LOOP, 0, side_count);
}

void platform::compute_bounding_box() {
  bounding_box[0] = corners[0];
  bounding_box[1] = corners[1];
  bounding_box[2] = bounding_box[0];
  bounding_box[3] = bounding_box[1];
  for(int i = 0; i < side_count; i++) {
    for(int j = 0; j < 2; j++) {
      if(bounding_box[j] > corners[i * 2 + j]) {
        bounding_box[j] = corners[i * 2 + j];
      }
      if(bounding_box[j + 2] < corners[i * 2 + j]) {
        bounding_box[j + 2] = corners[i * 2 + j];
      }
    }
  }
}