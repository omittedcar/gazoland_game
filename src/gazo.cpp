
#include "gl_or_gles.h"
#include <cstdio>
#include <cstring>
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>

#include "./gazo.h"

//esta es el caunto de lados del gazo
//por ejemplo, si fuera seis, el gazo es un hexagono
const int n_sides = 15;

//the angle between the spokes of the gazo
const double angle = 6.\
2831853071795864769252867665590057683943387987502116419498891846156328125724179\
9725606965068423413596429617302656461329418768921910116446345071881625696223490\
0568205403877042211119289245897909860763928857621951331866892256951296467573566\
3305424038182912971338469206972209086532964267872145204982825474491740132126311\
7634976304184192565850818343072873 / n_sides;

//the total number of vertices/nodes that the gzo has
const int n_verts = n_sides + 1u;

//the downard acceleration that the gazo experiences (㎨)
double gravity = 9.789615906;

// the air pressure that the gazo experiences (㎩)
double air_pressure = 0x18BCD;

//the gazo experiences pressure and pressure is force over area
//but because the gazo is a flat polygon, it has no surface area, only perimeter.
//this is the amount that it is extruded for the purpose of pressure
double fluid_interaction_width = 0.2;

//the mass of the gazo's inner node (㎏)
double inner_mass = 0x8;

//the combined masses of all of the other nodes. (㎏)
double outer_mass = 0xA;

//the stiffness of the gazo's outer edges. (N/m)
double outer_stiffness = 0x1800;

//the damping of the gazo's outer edges. (㎏/s)
double outer_damping = 0x60;

//the power of each of the gazo's muscles. (W)
double muscle_power = 0x1000;

double inner_stiffness = 0x800; //0x600
double inner_damping = 0x10;

//N
double internal_pressure_area = 0x3400;

double friction_coefficient = 1.0;

double drag_factor = 0x3;

double outer_vertex_mass = outer_mass / n_sides;

double dash_speed = 3.25;

double radius = 0.2;

double arc_distance = hypot(cos(angle) - 1, sin(angle));


void gazo::init() {
  glGenBuffers(1, &gl_vertex_buffer);
  glGenBuffers(1, &gl_element_index_buffer);
  glGenBuffers(1, &gl_uv_buffer);

  mapping = (vec2*) malloc(n_verts * sizeof(vec2));
  pos20 = (float*) malloc(n_verts * 2 * sizeof(float));


  pos                           = (vec2*) malloc(n_verts * sizeof(vec2));
  vel                           = (vec2*) malloc(n_verts * sizeof(vec2));
  sample_pos                    = (vec2*) malloc(n_verts * sizeof(vec2));
  sample_vel                    = (vec2*) malloc(n_verts * sizeof(vec2));
  acc                           = (vec2*) malloc(n_verts * sizeof(vec2));
  memset(acc, 0, n_verts * sizeof(vec2)); // 0x0000000000000000 is equal to 0.0
  delta_pos                     = (vec2*) malloc(n_verts * sizeof(vec2));
  delta_vel                     = (vec2*) malloc(n_verts * sizeof(vec2));



  ushort elements[45] = {
    0, 1, 2,
    0, 2, 3,
    0, 3, 4,
    0, 4, 5,
    0, 5, 6,
    0, 6, 7,
    0, 7, 8,
    0, 8, 9,
    0, 9, 10,
    0, 10, 11,
    0, 11, 12,
    0, 12, 13,
    0, 13, 14,
    0, 14, 15,
    0, 15, 1
  };

  mapping[0] = {0.0,0.0};

  for(uint i = 0u; i < n_sides; i++) {
    mapping[i+1].x = cos(angle * double(i));
    mapping[i+1].y = sin(angle * double(i));
  }

  fvec2* uv_map = (fvec2*) malloc(n_verts * 9 * sizeof(fvec2));
  for(int i = 0; i < 9; i++) {
    uv_map[i * n_verts] = {
      float((i%3-1)*(i%3-1)) * (i/3==1?0.7f:0.64f) + 0.25f,
      float(i/3-1)*(i%3==1?0.35f:0.32f) + 0.375f
    };
    for(int j = 0; j < n_sides; j++) {
      vec2 mapping_here = mapping[j + 1];
      fvec2 uv_value = {
        (float(mapping_here.x) + (i % 3 - 1) * 2) * (i % 3 == 0 ? -0.25f : 0.25f) + 0.25f,
        (float(-mapping_here.y) + (i / 3) * 2 + 1) * 0.125f
      };
      uv_map[i * n_verts + j + 1] = uv_value;
    }
  }
  glBindBuffer(GL_ARRAY_BUFFER, gl_uv_buffer);
  glBufferData(GL_ARRAY_BUFFER, n_verts * 9 * sizeof(fvec2), (float*)uv_map, GL_STATIC_DRAW);
  free(uv_map);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_element_index_buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, n_sides * 3 * sizeof(ushort), elements, GL_STATIC_DRAW);

  for(uint i = 0u; i < n_verts * 2u; i++) {
    ((double*)pos)[i] = ((double*)mapping)[i] * radius;
  }
  for(uint i = 0u; i < n_verts; i++) {
    pos[i].y++;
    vel[i].x = 0.0;
    vel[i].y = 0.0;
  }
}

bool gazo::advance_forward(double time_step) {

  double timestep_divided_by_sixe = time_step / 6.0;

  for (int i = 0; i < n_verts; i++) {
    delta_pos[i] = {0,0};
    delta_vel[i] = {0,0};
    sample_pos[i] = {0,0};
    sample_vel[i] = {0,0};
  }
  calculate_acc(pos, vel, acc);
  add_thing_to_other_thing((double*)vel, (double*)delta_pos, timestep_divided_by_sixe);
  add_thing_to_other_thing((double*)acc, (double*)delta_vel, timestep_divided_by_sixe);

  add_thing_to_other_thing_into_another_thing((double*)vel, (double*)pos, time_step * 0.5, (double*)sample_pos);
  add_thing_to_other_thing_into_another_thing((double*)acc, (double*)vel, time_step * 0.5, (double*)sample_vel);
  calculate_acc(sample_pos, sample_vel, acc);
  add_thing_to_other_thing((double*)sample_vel, (double*)delta_pos, timestep_divided_by_sixe * 2);
  add_thing_to_other_thing((double*)acc, (double*)delta_vel, timestep_divided_by_sixe * 2);


  add_thing_to_other_thing_into_another_thing((double*)sample_vel, (double*)pos, time_step * 0.5, (double*)sample_pos);
  add_thing_to_other_thing_into_another_thing((double*)acc, (double*)vel, time_step * 0.5, (double*)sample_vel);
  calculate_acc(sample_pos, sample_vel, acc);
  add_thing_to_other_thing((double*)sample_vel, (double*)delta_pos, timestep_divided_by_sixe * 2);
  add_thing_to_other_thing((double*)acc, (double*)delta_vel, timestep_divided_by_sixe * 2);


  add_thing_to_other_thing_into_another_thing((double*)sample_vel, (double*)pos, time_step, (double*)sample_pos);
  add_thing_to_other_thing_into_another_thing((double*)acc, (double*)vel, time_step, (double*)sample_vel);
  calculate_acc(sample_pos, sample_vel, acc);
  add_thing_to_other_thing((double*)sample_vel, (double*)delta_pos, timestep_divided_by_sixe);
  add_thing_to_other_thing((double*)acc, (double*)delta_vel, timestep_divided_by_sixe);

  add_thing_to_other_thing((double*)delta_pos, (double*)pos, 1.0);

  add_thing_to_other_thing((double*)delta_vel, (double*)vel, 1.0);

  return false;
}

fvec2 gazo::get_center_of_mass_medium_precision() {
  fvec2 output = {
    float(pos[0].x * inner_mass),
    float(pos[0].y * inner_mass)
  };
  for(int i = 0; i < n_sides; i++) {
    output.x += pos[i+1].x * outer_vertex_mass;
    output.y += pos[i+1].y * outer_vertex_mass;
  }
  output.x /= inner_mass + outer_mass;
  output.y /= inner_mass + outer_mass;
  return output;
}

void gazo::point_joystick(float x, float y) {
  pointing = {x,y};
}

void gazo::point_other_joystick(float x, float y) {
  vec2 dashing = {
    x - previous_joystick.x,
    y - previous_joystick.y
  };
  for(int i = 0; i < n_verts; i++) {
    vel[i].x += dashing.x * dash_speed;
    vel[i].y += dashing.y * dash_speed;
  }
  previous_joystick = {x,y};
}

void gazo::update_gl_vertex_buffer()  {
  for(uint i = 0u; i < n_verts; i++) {
    pos20[i*2u] = float(pos[i].x);
    pos20[i*2u+1u] = float(pos[i].y);
  }
  glBindBuffer(GL_ARRAY_BUFFER, gl_vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, n_verts * 2 * sizeof(float), pos20, GL_DYNAMIC_DRAW);
}

void gazo::update_gl_uv_buffer()  {
}

GLuint gazo::get_gl_vertex_buffer() {
  return gl_vertex_buffer;
}

double* gazo::get_mapping_pointer() {
  return (double*)mapping;
}

int gazo::get_vertex_buffer_size()  {
  return n_verts * 2 * sizeof(float);
}

void gazo::kill_to_death() {
  glDeleteBuffers(1, &gl_vertex_buffer);
  glDeleteBuffers(1, &gl_uv_buffer);
  glDeleteBuffers(1, &gl_element_index_buffer);
  free(mapping);
  free(pos);
  free(pos20);
  free(vel);
  free(delta_pos);
  free(delta_vel);
  free(sample_pos);
  free(sample_vel);
  free(acc);

}

void gazo::add_thing_to_other_thing(
  double* thing,
  double* other_thing,
  double koaficant
) {
  for(int i = 0; i < n_verts * 2; i++) {
    other_thing[i] += thing[i] * koaficant;
  }
}



/*function correct_collisions(time_step) {
  
  for(let i = 0; i < n_vertices; i++) {
    let p = [glaggle.position[i*2], glaggle.position[i*2+1]];
    if(current_level.collision_shape.is_colliding(p)) {
      let displacement = current_level.collision_shape.shortest_path(p);
      glaggle.position[i * 2] += displacement[0];
      glaggle.position[i * 2 + 1] += displacement[1];
      let velocity_change = [displacement[0] / time_step, displacement[1] / time_step];
      let displacement_length = Math.hypot(...displacement);
      let normal = [displacement[0] / displacement_length, displacement[1] / displacement_length];
      
      let velocity = [glaggle.velocity[i*2], glaggle.velocity[i*2+1]];
      
      let parallel_speed = velocity[0] * normal[1] - velocity[1] * normal[0];
      let speed_change = Math.hypot(...velocity_change);
      let friction_speed_loss = friction_coefficient * speed_change;
      let parallel_speed_after = Math.min(parallel_speed + friction_speed_loss, Math.max(parallel_speed - friction_speed_loss, 0));
      let parallel_speed_change = parallel_speed_after - parallel_speed;
      glaggle.velocity[i * 2] += parallel_speed_change * normal[1];
      glaggle.velocity[i * 2 + 1] -= parallel_speed_change * normal[0];
      
      glaggle.velocity[i * 2] += velocity_change[0];
      glaggle.velocity[i * 2 + 1] += velocity_change[1];
    }
  }
}*/

void gazo::push_out_from_platform(double interval, platform* pltfm) {
  for(int i = 0; i < n_verts; i++) {
    vec2 p = pos[i];
    
    //printf("  %f", p.y);
    if(pltfm->can_we_like_can_we_please_like_put_stuff_here_at_this_location_x_and_y_please_or_is_that_like_a_not_good_place_to_put_stuff_because_like_you_cant_put_stuff_there(p)) {
      vec2 displacement = pltfm->shortest_path(p);
      pos[i].x += displacement.x;
      pos[i].y += displacement.y;
      vec2 velocity_change = vec2{displacement.x / interval, displacement.y / interval};
      double displacement_length = hypot(displacement.x, displacement.y);
      vec2 normal = vec2{displacement.x / displacement_length, displacement.y / displacement_length};
      
      vec2 v = vel[i];
      
      double parallel_speed = v.x * normal.y - v.y * normal.x;
      double speed_change = displacement_length / interval;
      double friction_speed_loss = friction_coefficient * speed_change;
      double parallel_speed_after = fmin(parallel_speed + friction_speed_loss, fmax(parallel_speed - friction_speed_loss, 0));
      double parallel_speed_change = parallel_speed_after - parallel_speed;
      vel[i].x += parallel_speed_change * normal.y;
      vel[i].y -= parallel_speed_change * normal.x;
      
      vel[i].x += velocity_change.x;
      vel[i].y += velocity_change.y;
    }
    
  }
  //printf("\n");
}

void gazo::add_thing_to_other_thing_into_another_thing(
  double* thing,
  double* other_thing,
  double coefficient,
  double* another_thing
) {
  for(int i = 0; i < n_verts * 2; i++) {
    another_thing[i] = other_thing[i] + thing[i] * coefficient;
  }
}




void gazo::calculate_acc(vec2* pos_in, vec2* vel_in, vec2* acc_out) {
  for(int i = 0; i < n_verts; i++) {
    acc_out[i] = {0.0, -gravity};
  }

  double polygon_area = 0.0;
  for(int i = 0; i < n_sides; i++) {
    int j = (i + 1) % n_sides;
    polygon_area +=
      pos_in[i + 1].x * pos_in[j + 1].y -
      pos_in[i + 1].y * pos_in[j + 1].x
    ;
  }
  polygon_area *= 0.5;
  double total_pressure = internal_pressure_area / polygon_area - air_pressure;

  vec2 glaggle_rotation = {0, 0}; //as like a complex number.
  //for example {0, 1} means rotated counterclokwise 90 degress
  vec2 glaggle_center = pos_in[0];

  for(int i = 0; i < n_sides; i++) {
    glaggle_rotation.x +=
      +(pos_in[i + 1].x - glaggle_center.x) * mapping[i + 1].x
      +(pos_in[i + 1].y - glaggle_center.y) * mapping[i + 1].y
    ;
    glaggle_rotation.y +=
      -(pos_in[i + 1].x - glaggle_center.x) * mapping[i + 1].y
      +(pos_in[i + 1].y - glaggle_center.y) * mapping[i + 1].x
    ;
  }

  double glaggle_rotation_normaliser = 1 / hypot(glaggle_rotation.x, glaggle_rotation.y);
  glaggle_rotation.x *= glaggle_rotation_normaliser;
  glaggle_rotation.y *= glaggle_rotation_normaliser;

  vec2 rotated_joystick = {
    pointing.x * glaggle_rotation.x + pointing.y * glaggle_rotation.y,
    pointing.y * glaggle_rotation.x - pointing.x * glaggle_rotation.y
  };


  for(int i = 0; i < n_sides; i++) {
    vec2 v = {
      pos_in[i + 1].x - pos_in[0].x,
      pos_in[i + 1].y - pos_in[0].y
    };
    double current_length = hypot(v.x, v.y);
    double deformation_rate = (
      (vel_in[i + 1].x - vel_in[0].x) * v.x +
      (vel_in[i + 1].y - vel_in[0].y) * v.y
    ) / current_length;


    double target_muscle_force = (hypot(
      mapping[i + 1].x - rotated_joystick.x * 0.7,
      mapping[i + 1].y - rotated_joystick.y * 0.7
    ) - 1) * inner_stiffness * radius * 1;

    if(target_muscle_force * deformation_rate > muscle_power) {
      target_muscle_force = (muscle_power / deformation_rate) || 0;
    }


    double length_difference = current_length - radius;
    double force_quotient = (
      - length_difference * inner_stiffness
      - deformation_rate * inner_damping + target_muscle_force
    ) / (current_length);

    acc_out[0].x -= v.x * force_quotient / inner_mass;
    acc_out[0].y -= v.y * force_quotient / inner_mass;
    acc_out[i+1].x += v.x * force_quotient / outer_vertex_mass;
    acc_out[i+1].y += v.y * force_quotient / outer_vertex_mass;
  }



  for(int i = 0; i < n_sides; i++) {
    int j = (i + 1) % n_sides;
    vec2 v = {
      pos_in[j+1].x - pos_in[i+1].x,
      pos_in[j+1].y - pos_in[i+1].y
    };
    double current_length = hypot(v.x, v.y); 
    double deformation_rate = (
      (vel_in[j+1].x - vel_in[i+1].x) * v.x +
      (vel_in[j+1].y - vel_in[i+1].y) * v.y
    ) / current_length;


    double length_difference = current_length - arc_distance * radius;
    double force_quotient = (
      - length_difference * outer_stiffness
      - deformation_rate * outer_damping
    ) / (current_length);

    acc_out[i + 1].x -= v.x * force_quotient / outer_vertex_mass;
    acc_out[i + 1].y -= v.y * force_quotient / outer_vertex_mass;

    acc_out[j +1].x += v.x * force_quotient / outer_vertex_mass;
    acc_out[j +1].y += v.y * force_quotient / outer_vertex_mass;
  }

  for(int i = 0; i < n_sides; i++) {
    int j = (i + 1) % n_sides;
    vec2 v = {
      pos_in[j+1].x - pos_in[i+1].x,
      pos_in[j+1].y - pos_in[i+1].y,
    };

    double edge_length = hypot(v.x, v.y);
    vec2 normal_direction = {
      v.y / edge_length,
      -v.x / edge_length
    };

    vec2 endpoint_velocities[2] = {
      {vel_in[i+1].x, vel_in[i+1].y},
      {vel_in[j+1].x, vel_in[j+1].y}
    };

    double kinematic_pressures[2] = {
      (endpoint_velocities[0].x * normal_direction.x + endpoint_velocities[0].x * normal_direction.y) * hypot(endpoint_velocities[0].x, endpoint_velocities[0].x) * drag_factor,
      (endpoint_velocities[0].y * normal_direction.x + endpoint_velocities[1].y * normal_direction.y) * hypot(endpoint_velocities[1].x, endpoint_velocities[1].y) * drag_factor//i couldnt find the formuila for kinematic pressure so I made one up :)
    };


    double pressure_acc_factor = 0.5 / outer_vertex_mass * fluid_interaction_width;

    acc_out[i +1].x += v.y * (total_pressure) * pressure_acc_factor;
    acc_out[i +1].y -= v.x * (total_pressure) * pressure_acc_factor;

    acc_out[j +1].x += v.y * (total_pressure) * pressure_acc_factor;
    acc_out[j +1].y -= v.x * (total_pressure) * pressure_acc_factor;
  }
}












float gazo::get_rumble() {
  /*
  float rumble_out = 0.0;
  for(int i = 0; i < n_sides; i++) {
    int j = (i+1)%n_sides;
    double v[2] = {
      pos[i * 2 + 2] - pos[j*2+2],
      pos[i * 2 + 3] - pos[j*2+3]
    };
    double current_length = hypot(v[0], v[1]);
    double deformation_rate = (
      (vel[i * 2 + 2] - vel[j*2+2]) * v[0] +
      (vel[i * 2 + 3] - vel[j*2+3]) * v[1]
    ) / current_length;
    rumble_out += deformation_rate * deformation_rate;
  }



  for(int i = 0; i < n_sides; i++) {
    int j = (i + 1) % n_sides;
    double v[2] = {
      pos[j * 2 + 2] - pos[i * 2 + 2],
      pos[j * 2 + 3] - pos[i * 2 + 3]
    };
    double current_length = hypot(v[0], v[1]);
    double deformation_rate = (
      (vel[j * 2 + 2] - vel[i * 2 + 2]) * v[0] +
      (vel[j * 2 + 3] - vel[i * 2 + 3]) * v[1]
    ) / current_length;
  }
  return rumble_out;
  */
  return 0;
}

void gazo::render(
  gl_program_info* shader
) {
  blink_timer++;
  glDisable(GL_CULL_FACE);

  update_gl_vertex_buffer();
  update_gl_uv_buffer();
  glUseProgram(shader->shader);

  glBindBuffer(GL_ARRAY_BUFFER, gl_vertex_buffer);
  glVertexAttribPointer(shader->v_pos, 2, GL_FLOAT, false, 0, nullptr);
  glEnableVertexAttribArray(shader->v_pos);


  choose_sprite();
  glBindBuffer(GL_ARRAY_BUFFER, gl_uv_buffer);
  glVertexAttribPointer(shader->v_uv, 2, GL_FLOAT, false, 0, (void*) (long long int) (uv_map_offset * 0x80));
  glEnableVertexAttribArray(shader->v_uv);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_element_index_buffer);
  glLineWidth(2);

  glDrawElements(GL_TRIANGLES, n_sides * 3, GL_UNSIGNED_SHORT, nullptr);
  glEnable(GL_BLEND);
  glBlendColor(0.0, 0.0, 0.0, 0.5);
  glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
  glBindTexture(GL_TEXTURE_2D, 0);

  glDrawArrays(GL_LINE_LOOP, 1, n_sides);
  glDisable(GL_BLEND);
  glLineWidth(1);
  glDrawArrays(GL_LINE_LOOP, 1, n_sides);

  glDisableVertexAttribArray(shader->v_uv);
  glDisableVertexAttribArray(shader->v_pos);
}

void gazo::choose_sprite() {
  vec2 rotation = {0, 0};
  vec2 center = pos[0];

  for(int i = 0; i < n_sides; i++) {
    rotation.x +=
      +(pos[i + 1].x - center.x) * mapping[i + 1].x
      +(pos[i + 1].y - center.y) * mapping[i + 1].y
    ;
    rotation.y +=
      -(pos[i + 1].x - center.x) * mapping[i + 1].y
      +(pos[i + 1].y - center.y) * mapping[i + 1].x
    ;
  }

  double rotation_normaliser = 1 / hypot(rotation.x, rotation.y);
  rotation.x *= rotation_normaliser;
  rotation.y *= rotation_normaliser;
  vec2 rotated_joystick = {
    pointing.x * rotation.x + pointing.y * rotation.y,
    pointing.y * rotation.x - pointing.x * rotation.y
  };
  if(pointing.x * pointing.x + pointing.y * pointing.y < 0.5) {
    uv_map_offset = 4;
  } else {
    float pointing_normalizer = 1 / hypot(pointing.x, pointing.y);
    uv_map_offset = int(-rotated_joystick.y * pointing_normalizer * 1.5 + 1) * 3;
    uv_map_offset += int(rotated_joystick.x * pointing_normalizer * 1.5 + 1);
  }
}
