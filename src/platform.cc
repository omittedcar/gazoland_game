#include "platform.h"
#include "gl_or_gles.h"
#include <cmath>
#include <stdlib.h>
#include <math.h>

void platform::arise() {
  side_count = 9;
  corners = (fvec2*) malloc(side_count * 2 * sizeof(fvec2));
  corners[0] = {-0.875,-1.0};
  corners[1] = {-1.0,-1.125};
  corners[2] = {-1.0,-2.875};
  corners[3] = {-0.875,-3.0};
  corners[4] = {0.875,-3.0};
  corners[5] = {1.0,-2.875};
  corners[6] = {1.0,-1.125};
  corners[7] = {0.875,-1.0};
  corners[8] = {0.0, -1.25};
  compute_bounding_box();
  glGenBuffers(1, &vertex_pos_buffer);
  glGenBuffers(1, &vertex_uv_buffer);
  glGenBuffers(1, &face_index_buffer);
  do_vertex_buffers();
}

void platform::demolish() {
  free(corners);
  glDeleteBuffers(1, &vertex_pos_buffer);
  glDeleteBuffers(1, &vertex_uv_buffer);
  glDeleteBuffers(1, &face_index_buffer);
}

void platform::draw(
  gl_program_info* rendering_shader
) {
  
  glUseProgram(rendering_shader->shader);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_pos_buffer);
  glVertexAttribPointer(rendering_shader->v_pos, 3, GL_FLOAT, false, 0, nullptr);
  glEnableVertexAttribArray(rendering_shader->v_pos);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_uv_buffer);
  glVertexAttribPointer(rendering_shader->v_uv, 2, GL_FLOAT, false, 0, nullptr);
  glEnableVertexAttribArray(rendering_shader->v_uv);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, face_index_buffer);
  glDrawElements(GL_TRIANGLES, side_count * 18, GL_UNSIGNED_SHORT, nullptr);
}

void platform::compute_bounding_box() {
  bounding_box[0] = corners[0];
  bounding_box[1] = corners[0];
  for(int i = 0; i < side_count; i++) {
    if(bounding_box[0].x > corners[i].x) {
      bounding_box[0].x = corners[i].x;
    }
    if(bounding_box[0].y > corners[i].y) {
      bounding_box[0].y = corners[i].y;
    }
    if(bounding_box[1].x < corners[i].x) {
      bounding_box[1].x = corners[i].x;
    }
    if(bounding_box[1].y < corners[i].y) {
      bounding_box[1].y = corners[i].y;
    }
  }
}
bool platform::can_we_like_can_we_please_like_put_stuff_here_at_this_location_x_and_y_please_or_is_that_like_a_not_good_place_to_put_stuff_because_like_you_cant_put_stuff_there(vec2 p) {
  bool is_colliding = false;
  for(int i = 0; i < side_count; i++) {
    int j = (i + 1) % side_count;
    if(p.y > fmin(corners[i].y, corners[j].y)) {
      if(p.y <= fmax(corners[i].y, corners[j].y)) {
        if(p.x <= fmax(corners[i].x,corners[j].x)) {
          double x_intercept = (p.y - corners[i].y) * (corners[j].x - corners[i].x) / (corners[j].y - corners[i].y) + corners[i].x;
          if(corners[i].x == corners[j].x || p.x <= x_intercept) {
            is_colliding = !is_colliding;
          }
        }
      }
    }
  }
  //return p.y < -1;
  return is_colliding;
}
vec2 platform::shortest_path(vec2 p0) {
  vec2 path = {0,0};
  double distance_2 = HUGE_VAL_F64;
  for(int i = 0; i < side_count; i++) {
    int j = (i + 1) % side_count;
    fvec2 p1 = corners[i];
    fvec2 p2 = corners[j];
    vec2 a2b = {p2.x - p1.x, p2.y - p1.y};
    vec2 a2p = {p0.x - p1.x, p0.y - p1.y};
    double len = a2b.x * a2b.x + a2b.y * a2b.y;
    double dot = (a2p.x * a2b.x) + (a2p.y * a2b.y);
    double t = fmin( 1, fmax( 0, dot / len ) );
    vec2 canidate = {p1.x + a2b.x * t - p0.x,p1.y + a2b.y * t - p0.y};
    double candidate_distance_2 = canidate.x * canidate.x + canidate.y * canidate.y;
    if(candidate_distance_2 < distance_2) {
      distance_2 = candidate_distance_2;
      path = canidate;
    }
  }
  return path;
  //return {0.0, -1 - p0.y};
}

void platform::do_vertex_buffers() {
  float* vertexes = (float*) malloc(side_count * 24 * sizeof(float));
  float* uvs = (float*) malloc(side_count * 16 * sizeof(float));
  unsigned short* indexes = (unsigned short*) malloc(side_count * 18 * sizeof(unsigned short));
  float uv_x = 0.0;
  float uv_x_next = 0.0;

  for(int i = 0; i < side_count; i++) {
    fvec2 corner_a = corners[i];
    fvec2 corner_b = corners[(i+1)%side_count];
    fvec2 a_to_b = {
      corner_b.x - corner_a.x,
      corner_b.y - corner_a.y
    };
    float distance_accross = hypot(a_to_b.x, a_to_b.y);
    float inverse_distacne = 1.0 / distance_accross;
    fvec2 normal = {
      a_to_b.y * inverse_distacne,
      -a_to_b.x * inverse_distacne
    };
    uv_x_next = uv_x + distance_accross / 1.5;

    vertexes[i * 24] = corner_a.x + normal.x * 0.25;
    vertexes[i * 24 + 1] = corner_a.y + normal.y * 0.25;
    vertexes[i * 24 + 2] = 0.0;
    uvs[i * 16] = uv_x;
    uvs[i * 16 + 1] = 0.0;

    vertexes[i * 24 + 3] = corner_b.x + normal.x * 0.25;
    vertexes[i * 24 + 4] = corner_b.y + normal.y * 0.25;
    vertexes[i * 24 + 5] = 0.0;
    uvs[i * 16 + 2] = uv_x_next;
    uvs[i * 16 + 3] = 0.0;

    vertexes[i * 24 + 6] = corner_a.x;
    vertexes[i * 24 + 7] = corner_a.y;
    vertexes[i * 24 + 8] = -0.25;
    uvs[i * 16 + 4] = uv_x;
    uvs[i * 16 + 5] = 0.33333251;

    vertexes[i * 24 + 9] = corner_b.x;
    vertexes[i * 24 + 10] = corner_b.y;
    vertexes[i * 24 + 11] = -0.25;
    uvs[i * 16 + 6] = uv_x_next;
    uvs[i * 16 + 7] = 0.33333251;

    vertexes[i * 24 + 12] = corner_a.x - normal.x * 0.25;
    vertexes[i * 24 + 13] = corner_a.y - normal.y * 0.25;
    vertexes[i * 24 + 14] = 0.0;
    uvs[i * 16 + 8] = uv_x;
    uvs[i * 16 + 9] = 0.66668324;

    vertexes[i * 24 + 15] = corner_b.x - normal.x * 0.25;
    vertexes[i * 24 + 16] = corner_b.y - normal.y * 0.25;
    vertexes[i * 24 + 17] = 0.0;
    uvs[i * 16 + 10] = uv_x_next;
    uvs[i * 16 + 11] = 0.66668324;

    vertexes[i * 24 + 18] = corner_a.x - normal.x * 0.5;
    vertexes[i * 24 + 19] = corner_a.y - normal.y * 0.5;
    vertexes[i * 24 + 20] = 0.25;
    uvs[i * 16 + 12] = uv_x;
    uvs[i * 16 + 13] = 1.0;

    vertexes[i * 24 + 21] = corner_b.x - normal.x * 0.5;
    vertexes[i * 24 + 22] = corner_b.y - normal.y * 0.5;
    vertexes[i * 24 + 23] = 0.25;
    uvs[i * 16 + 14] = uv_x_next;
    uvs[i * 16 + 15] = 1.0;

    indexes[i * 3] = i * 8 + 2;
    indexes[i * 3+1] = i * 8 + 3;
    indexes[i * 3+2] = i * 8 + 4;

    indexes[i * 3 + side_count * 3] = i * 8 + 1;
    indexes[i * 3+ side_count * 3+1] = i * 8 + 2;
    indexes[i * 3+ side_count * 3+2] = i * 8 + 3;

    indexes[i * 3 + side_count * 6] = i * 8 + 3;
    indexes[i * 3 + side_count * 6 +1] = i * 8 + 4;
    indexes[i * 3 + side_count * 6 +2] = i * 8 + 5;

    indexes[i * 3 + side_count * 9] = i * 8 + 0;
    indexes[i * 3 + side_count * 9 +1] = i * 8 + 1;
    indexes[i * 3 + side_count * 9 +2] = i * 8 + 2;

    indexes[i * 3 + side_count * 12] = i * 8 + 4;
    indexes[i * 3 + side_count * 12 +1] = i * 8 + 5;
    indexes[i * 3 + side_count * 12 +2] = i * 8 + 6;

    indexes[i * 3 + side_count * 15] = i * 8 + 5;
    indexes[i * 3 + side_count * 15 +1] = i * 8 + 6;
    indexes[i * 3 + side_count * 15 +2] = i * 8 + 7;

    uv_x = uv_x_next;
  }

  glBindBuffer(GL_ARRAY_BUFFER, vertex_pos_buffer);
  glBufferData(GL_ARRAY_BUFFER, side_count * 24 * sizeof(float), vertexes, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_uv_buffer);
  glBufferData(GL_ARRAY_BUFFER, side_count * 16 * sizeof(float), uvs, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, face_index_buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, side_count * 18 * sizeof(unsigned short), indexes, GL_STATIC_DRAW);
  free(vertexes);
  free(uvs);
  free(indexes);
}