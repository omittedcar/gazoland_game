#include "platform.h"
#include "gl_or_gles.h"
#include <cmath>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

void platform::arise(fvec2* corners_in, int side_count_in) {
  side_count = side_count_in;
  corners = corners_in;

  compute_bounding_box();
  glGenBuffers(6, &vertex_uv_buffer);
  do_vertex_buffers();
  generate_mesh();
}

void platform::demolish() {
  free(corners);
  glDeleteBuffers(6, &vertex_uv_buffer);
}

void platform::draw(
  gl_program_info* surface_shader,
  gl_program_info* fill_shader,
  float* projection,
  fvec2 view
) {
  
  glUseProgram(surface_shader->program);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_pos_buffer);
  glVertexAttribPointer(surface_shader->v_pos, 3, GL_FLOAT, false, 0, nullptr);
  glEnableVertexAttribArray(surface_shader->v_pos);
  
  glBindBuffer(GL_ARRAY_BUFFER, vertex_uv_buffer);
  glVertexAttribPointer(surface_shader->v_uv, 2, GL_FLOAT, false, 0, nullptr);
  glEnableVertexAttribArray(surface_shader->v_uv);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, upper_surface_index_buffer);
  glDrawElements(GL_TRIANGLES, side_count * 6, GL_UNSIGNED_SHORT, nullptr);
  glDisableVertexAttribArray(surface_shader->v_pos);
  glDisableVertexAttribArray(surface_shader->v_uv);
  
  //glDisable(GL_BLEND);
  glUseProgram(fill_shader->program);
  glEnableVertexAttribArray(fill_shader->v_pos);
  glBindBuffer(GL_ARRAY_BUFFER, corner_vertex_buffer);
  glVertexAttribPointer(fill_shader->v_pos, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  glUniformMatrix4fv(fill_shader->u_projection, 1, false, projection);
  glUniform1i(fill_shader->u_texture, 0);
  glUniform2f(fill_shader->u_panning, view.x, view.y);

  //glEnable(GL_BLEND);
  
  glBindTexture(GL_TEXTURE_2D, 6);
  glDisable(GL_CULL_FACE);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, inner_face_index_buffer);
  glDrawElements(GL_TRIANGLES, 3*(side_count - 2), GL_UNSIGNED_SHORT, nullptr);

  glUseProgram(surface_shader->program);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_pos_buffer);
  glVertexAttribPointer(surface_shader->v_pos, 3, GL_FLOAT, false, 0, nullptr);
  glEnableVertexAttribArray(surface_shader->v_pos);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_uv_buffer);
  
  glVertexAttribPointer(surface_shader->v_uv, 2, GL_FLOAT, false, 0, nullptr);
  glEnableVertexAttribArray(surface_shader->v_uv);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lower_surface_index_buffer);
  //glEnable(GL_BLEND);
  glBindTexture(GL_TEXTURE_2D, 5);
  glDrawElements(GL_TRIANGLES, side_count * 6, GL_UNSIGNED_SHORT, nullptr);
  glDisableVertexAttribArray(surface_shader->v_pos);
  glDisableVertexAttribArray(surface_shader->v_uv);
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
  //one rectangle-pair per side, each rectangle-pair is 6 verts.
  //so the vertex count is the side count times six
  float* vertexes = (float*) malloc(side_count * 18 * sizeof(float)); 
  float* uvs = (float*) malloc(side_count * 12 * sizeof(float));
  //two triangles per rectangle, three indexes per triangle.
  unsigned short* indexes = (unsigned short*) malloc(side_count * 6 * sizeof(unsigned short));
  unsigned short* indexes_b = (unsigned short*) malloc(side_count * 6 * sizeof(unsigned short));
  float uv_x = 0.0;
  float uv_x_next = 0.0;

  //the surfaces of the platform occupy the interval of depth from 
  //from -0.25 to 0.0

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
    uv_x_next = uv_x + distance_accross / 1.0;

    //the first two vertices of the rectangle pair are 375mm above the surface:
    vertexes[i * 18] = corner_a.x + normal.x * 0.375;
    vertexes[i * 18 + 1] = corner_a.y + normal.y * 0.375;
    vertexes[i * 18 + 2] = 0.0;
    uvs[i * 12] = uv_x;
    uvs[i * 12 + 1] = 0.0;
    vertexes[i * 18 + 3] = corner_b.x + normal.x * 0.375;
    vertexes[i * 18 + 4] = corner_b.y + normal.y * 0.375;
    vertexes[i * 18 + 5] = 0.0;
    uvs[i * 12 + 2] = uv_x_next;
    uvs[i * 12 + 3] = 0.0;

//    1==--------------0
//    |  ^^^^----____  |
//    3==------------**2
//    |  ^^^^----____  |
//    5--------------**4
//

// the next two are 125mm below the surface
    vertexes[i * 18 + 6] = corner_a.x - normal.x * .125;
    vertexes[i * 18 + 7] = corner_a.y - normal.y * .125;
    vertexes[i * 18 + 8] = -0.25;
    uvs[i * 12 + 4] = uv_x;
    uvs[i * 12 + 5] = 0.5;

    vertexes[i * 18 + 9] = corner_b.x - normal.x * .125;
    vertexes[i * 18 + 10] = corner_b.y - normal.y * .125;
    vertexes[i * 18 + 11] = -0.25;
    uvs[i * 12 + 6] = uv_x_next;
    uvs[i * 12 + 7] = 0.5;

// and the last two are 625mm below the surface
    vertexes[i * 18 + 12] = corner_a.x - normal.x * 0.625;
    vertexes[i * 18 + 13] = corner_a.y - normal.y * 0.625;
    vertexes[i * 18 + 14] = 0.0;
    uvs[i * 12 + 8] = uv_x;
    uvs[i * 12 + 9] = 1.0;

    vertexes[i * 18 + 15] = corner_b.x - normal.x * 0.625;
    vertexes[i * 18 + 16] = corner_b.y - normal.y * 0.625;
    vertexes[i * 18 + 17] = 0.0;
    uvs[i * 12 + 10] = uv_x_next;
    uvs[i * 12 + 11] = 1.0;

    indexes[i * 3] = i * 6 + 2;
    indexes[i * 3+1] = i * 6 + 1;
    indexes[i * 3+2] = i * 6 + 3;

    indexes[i * 3 + side_count * 3] = i * 6 + 2;
    indexes[i * 3 + side_count * 3 + 1] = i * 6 + 0;
    indexes[i * 3 + side_count * 3 + 2] = i * 6 + 1;

    indexes_b[i * 3 ] = i * 6 + 4;
    indexes_b[i * 3 +1] = i * 6 + 2;
    indexes_b[i * 3 +2] = i * 6 + 3;

    indexes_b[i * 3 + side_count * 3] = i * 6 + 4;
    indexes_b[i * 3 + side_count * 3 +1] = i * 6 + 3;
    indexes_b[i * 3 + side_count * 3 +2] = i * 6 + 5;

    uv_x = uv_x_next;
  }

  glBindBuffer(GL_ARRAY_BUFFER, vertex_pos_buffer);
  glBufferData(GL_ARRAY_BUFFER, side_count * 18 * sizeof(float), vertexes, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_uv_buffer);
  glBufferData(GL_ARRAY_BUFFER, side_count * 12 * sizeof(float), uvs, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, corner_vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, side_count * sizeof(fvec2), (float*) corners, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, upper_surface_index_buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, side_count * 6 * sizeof(unsigned short), indexes, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lower_surface_index_buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, side_count * 6 * sizeof(unsigned short), indexes_b, GL_STATIC_DRAW);
  
  free(vertexes);
  free(uvs);
  free(indexes);
  free(indexes_b);
}

void platform::generate_mesh() {
  signed short* mesh_which_we_are_generating =
    (signed short*) malloc((side_count-2) * 3 * sizeof(unsigned short) + 1)
  ;
  signed short* unclipped_corner_indexes =
    (signed short*) malloc((side_count) * sizeof(unsigned short) + 1)
  ;
  signed short unclipped_corner_count = side_count;
  for(int i = 0; i < side_count; i++) {
    unclipped_corner_indexes[i] = i;
  }
  int triangle_count = 0;
  while(unclipped_corner_count > 3) {
    signed short ear = 0;
    for(; ear < unclipped_corner_count; ear++) {
      //get the the coordinates of the clockwise corner of the potential ear
      fvec2 corner_cw = corners[
        unclipped_corner_indexes[(ear + unclipped_corner_count - 1)%unclipped_corner_count]
      ]; //adding unclipped_corner_count to the number to keep it positive (poor handling of negative numbers)
      //get the coordinates of the central corner of the potential ear
      fvec2 corner = corners[unclipped_corner_indexes[ear]];
      //get the coordinates of the counter-clockwise corner of the potential ear
      fvec2 corner_ccw = corners[unclipped_corner_indexes[(ear + 1)%unclipped_corner_count]];
      if( //initial ear check: is it convex (if not then it isn't an ear)
        (corner_cw.y - corner.y) * (corner_ccw.x - corner.x) -
        (corner_cw.x - corner.x) * (corner_ccw.y - corner.y)
        > 0.0
      ) {
        //go through all of the unclipped corners that are not part of the potential ear
        //to see if they poke inside of it
        //which means that it isn't an ear
        bool is_poking = false;
        for(int i = 0; i < unclipped_corner_count - 3; i++) {
          fvec2 corner_checking = corners[
            unclipped_corner_indexes[(ear + 2 + i)%unclipped_corner_count]
          ];
          if(
            (corner_checking.y - corner.y) * (corner_cw.x - corner.x) -
            (corner_checking.x - corner.x) * (corner_cw.y - corner.y) < 0 &&

            (corner_checking.y - corner_ccw.y) * (corner.x - corner_ccw.x) -
            (corner_checking.x - corner_ccw.x) * (corner.y - corner_ccw.y) < 0 &&

            (corner_checking.y - corner_cw.y) * (corner_ccw.x - corner_cw.x) -
            (corner_checking.x - corner_cw.x) * (corner_ccw.y - corner_cw.y) < 0
          ) {
            is_poking = true;
          }
        }
        if(!is_poking) {
          break; //ear has been found. end of ear search.
        }
      } //end of ear check
    } //end of ear search
    mesh_which_we_are_generating[triangle_count*3] = unclipped_corner_indexes[ear];
    mesh_which_we_are_generating[triangle_count*3+1] = unclipped_corner_indexes[
      (ear+unclipped_corner_count-1)%unclipped_corner_count
    ];
    mesh_which_we_are_generating[triangle_count*3+2] = unclipped_corner_indexes[
      (ear+1)%unclipped_corner_count
    ];
    triangle_count++;
    unclipped_corner_count--;
    for(int i = 0; i < unclipped_corner_count - ear; i++) {
      unclipped_corner_indexes[ear + i] = unclipped_corner_indexes[ear+i+1];
    }
  }
  //remember: that was a while loop where the condition was that unclipped_corner_count > 3
  //so now that the loop has ended, we are left with one last triangle
  //let's add it to the mesh
  mesh_which_we_are_generating[triangle_count*3] = unclipped_corner_indexes[0];
  mesh_which_we_are_generating[triangle_count*3+1] = unclipped_corner_indexes[1];
  mesh_which_we_are_generating[triangle_count*3+2] = unclipped_corner_indexes[2];

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, inner_face_index_buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, (side_count-2) * 3 * sizeof(unsigned short), mesh_which_we_are_generating, GL_STATIC_DRAW);
  free(mesh_which_we_are_generating);
}
