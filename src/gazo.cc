
#include <cstring>
#include <stdlib.h>
#include <GLES3/gl3.h>
#include <math.h>


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
double fluid_interaction_width = 0.1;

//the mass of the gazo's inner node (㎏)
double inner_mass = 0x8;

//the combined masses of all of the other nodes. (㎏)
double outer_mass = 0xA;

//the stiffness of the gazo's outer edges. (N/m)
double outer_stiffness = 0x1000; //0x1000

//the damping of the gazo's outer edges. (㎏/s)
double outer_damping = 0x20;

//the power of each of the gazo's muscles. (W)
double muscle_power = 0x200;

double inner_stiffness = 0x60; //0x600
double inner_damping = 0xA; //0xA

//N
double internal_pressure_area = 0x3400;

double friction_coefficient = 1.0;

double drag_factor = 0x3;

double outer_vertex_mass = outer_mass / n_sides;

double dash_speed = 3.5;

double radius = 0.2;

double arc_distance = hypot(cos(angle) - 1, sin(angle));

void gazo::init() {
  glGenBuffers(1, &gl_vertex_buffer);
  mapping = (double*) malloc(n_verts * 2u * sizeof(double));
  pos20 = (float*) malloc(n_verts * 2u * sizeof(float));


  pos                           = (double*) malloc(n_verts * 2u * sizeof(double));
  vel                           = (double*) malloc(n_verts * 2u * sizeof(double));
  sample_pos                    = (double*) malloc(n_verts * 2u * sizeof(double));
  sample_vel                    = (double*) malloc(n_verts * 2u * sizeof(double));
  acc                           = (double*) malloc(n_verts * 2u * sizeof(double));
  memset(acc, 0, n_verts * 2 * sizeof(double));
  delta_pos                     = (double*) malloc(n_verts * 2u * sizeof(double));
  delta_vel                     = (double*) malloc(n_verts * 2u * sizeof(double));

  muscle_forces = (double*) malloc(n_sides * sizeof(double));

  elements = (ushort*) malloc(n_verts * 3 * sizeof(ushort));

  mapping[0] = 0.0;
  mapping[1] = 0.0;
  for(uint i = 0u; i < n_sides; i++) {
    mapping[i*2u+2u] = cos(angle * double(i));
    mapping[i*2u+3u] = sin(angle * double(i));
    elements[i*3] = 0;
    elements[(i*3+1)] = i;
    elements[(i*3+2)] = (i+1) % n_sides;
  }
  for(uint i = 0u; i < n_verts * 2u; i++) {
    pos[i] = mapping[i] * radius;
    if(i % 2 == 1) {
      pos[i]++;
    }
  }
  for(uint i = 0u; i < n_verts; i++) {
    vel[i * 2] = 1.0;
    vel[i * 2 + 1] = -0x10;
  }
}

bool gazo::advance_forward(double time_step) {

  double timestep_divided_by_sixe = time_step / 6.0;

  for (int i = 0; i < n_verts * 2; i++) {
    delta_pos[i] = 0;
    delta_vel[i] = 0;
    sample_pos[i] = 0;
    sample_vel[i] = 0;
  }
  calculate_acc(pos, vel, acc);
  add_thing_to_other_thing(acc, delta_pos, timestep_divided_by_sixe);
  add_thing_to_other_thing(acc, delta_vel, timestep_divided_by_sixe);

  add_thing_to_other_thing_into_another_thing(vel, pos, time_step * 0.5, sample_pos);
  add_thing_to_other_thing_into_another_thing(acc, vel, time_step * 0.5, sample_vel);
  calculate_acc(sample_pos, sample_vel, acc);
  add_thing_to_other_thing(sample_vel, delta_pos, timestep_divided_by_sixe * 2);
  add_thing_to_other_thing(acc, delta_vel, timestep_divided_by_sixe * 2);


  add_thing_to_other_thing_into_another_thing(sample_vel, pos, time_step * 0.5, sample_pos);
  add_thing_to_other_thing_into_another_thing(acc, vel, time_step * 0.5, sample_vel);
  calculate_acc(sample_pos, sample_vel, acc);
  add_thing_to_other_thing(sample_vel, delta_pos, timestep_divided_by_sixe * 2);
  add_thing_to_other_thing(acc, delta_vel, timestep_divided_by_sixe * 2);


  add_thing_to_other_thing_into_another_thing(sample_vel, pos, time_step, sample_pos);
  add_thing_to_other_thing_into_another_thing(acc, vel, time_step, sample_vel);
  calculate_acc(sample_pos, sample_vel, acc);
  add_thing_to_other_thing(acc, delta_pos, timestep_divided_by_sixe);
  add_thing_to_other_thing(acc, delta_vel, timestep_divided_by_sixe);

  add_thing_to_other_thing(delta_pos, pos, 1.0);
  add_thing_to_other_thing(delta_vel, vel, 1.0);

  if(is_in_wall()) {
    push_out_from_wall(time_step);
    return true;
  }
  return false;
}

void gazo::update_gl_vertex_buffer()  {
  for(uint i = 0u; i < n_verts; i++) {
    pos20[i*2u] = float(pos[i*2u]);
    pos20[i*2u+1u] = float(pos[i*2u+1]);
  }
  glBindBuffer(GL_ARRAY_BUFFER, gl_vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, n_verts * 2 * sizeof(float), pos20, GL_DYNAMIC_DRAW);
}

GLuint gazo::get_gl_vertex_buffer() {
  return gl_vertex_buffer;
}

ushort* gazo::get_element_pointer() {
  return elements;
}

double* gazo::get_mapping_pointer() {
  return mapping;
}

int gazo::get_vertex_buffer_size()  {
  return n_verts * 2 * sizeof(float);
}

void gazo::kill_to_death() {
  free(mapping);
  free(pos);
  free(pos20);
  free(elements);
  free(vel);
  free(delta_pos);
  free(delta_vel);
  free(sample_pos);
  free(sample_vel);
  free(acc);
  free(muscle_forces);
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

bool gazo::is_in_wall() {
  double bounding_box[4] = {
    INFINITY, INFINITY,
    -INFINITY, -INFINITY
  };
  for(int i = 0; i < n_verts; i++) {
    bounding_box[0] = fmin(pos[i * 2], bounding_box[0]);
    bounding_box[1] = fmin(pos[i * 2 + 1], bounding_box[1]);
    bounding_box[2] = fmax(pos[i * 2], bounding_box[2]);
    bounding_box[3] = fmax(pos[i * 2 + 1], bounding_box[3]);
  }
  return bounding_box[1] < -2;
}

void gazo::push_out_from_wall(double time_since_not_in_wall) {
  double wall_push_factor = 1 / time_since_not_in_wall;
  for(int i = 0; i < n_verts; i++) {
    double x = pos[i * 2];
    double y = pos[i * 2 + 1];
    if(y < -2) {
      double vx = vel[i * 2];
      double vy = vel[i * 2 + 1];
      double depth_in_wall = -y - 2;
      double impact = depth_in_wall * wall_push_factor;
      pos[i * 2 + 1] = -2;
      vel[i * 2 + 1] += impact;
      double delta_sliding = fmin(impact, fmax(-vx, -impact));
      pos[i * 2] += delta_sliding * time_since_not_in_wall;
      vel[i * 2] += delta_sliding;
    }
  }
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




void gazo::calculate_acc(double* pos_in, double* vel_in, double* acc_out) {
  for(int i = 0; i < n_verts; i++) {
    acc_out[i * 2] = 0.0;
    acc_out[i * 2 + 1] = gravity;
  }

  double polygon_area = 0.0;
  for(int i = 0; i < n_sides; i++) {
    int j = (i + 1) % n_sides;
    polygon_area += (pos[i * 2 + 2] - pos[j * 2 + 2])
    * (pos[i * 2 + 3] + pos[j * 2 + 3]) * 0.5;
  }

  double total_pressure = (internal_pressure_area / polygon_area - air_pressure) / (signbit(polygon_area) ? -1 : 1);

  double glaggle_rotation[2] = {0, 0}; //as like a complex number.
  //for example {0, 1} means rotated counterclokwise 90 degress
  double glaggle_center[2] = {pos[0], pos[1]};

  for(int i = 0; i < n_sides; i++) {
    glaggle_rotation[0] +=
      +(pos[i * 2 + 2] - glaggle_center[0]) * mapping[i * 2 + 2]
      +(pos[i * 2 + 3] - glaggle_center[1]) * mapping[i * 2 + 3]
    ;
    glaggle_rotation[1] +=
      -(pos[i * 2 + 2] - glaggle_center[0]) * mapping[i * 2 + 3]
      +(pos[i * 2 + 3] - glaggle_center[1]) * mapping[i * 2 + 2]
    ;
  }

  double glaggle_rotation_normaliser = 1 / hypot(glaggle_rotation[0], glaggle_rotation[1]);
  glaggle_rotation[0] *= glaggle_rotation_normaliser;
  glaggle_rotation[1] *= glaggle_rotation_normaliser;

  double rotated_joystick[2] = {
    pointing[0] * glaggle_rotation[0] + pointing[1] * glaggle_rotation[1],
    pointing[1] * glaggle_rotation[0] - pointing[0] * glaggle_rotation[1]
  };

  for(int i = 0; i < n_sides; i++) {
    muscle_forces[i] = (hypot(
      mapping[i * 2 + 2] - rotated_joystick[0] * 0.625,
      mapping[i * 2 + 3] - rotated_joystick[1] * 0.625
    ) - 1) * inner_stiffness * radius * 1;
  }

  for(int i = 0; i < n_sides; i++) {
    double v[2] = {
      pos[i * 2 + 2] - pos[0],
      pos[i * 2 + 3] - pos[1]
    };
    double current_length = hypot(v[0], v[1]);
    double deformation_rate = (
      (vel[i * 2 + 2] - vel[0]) * v[0] +
      (vel[i * 2 + 3] - vel[1]) * v[1]
    ) / current_length;


    double target_muscle_force = muscle_forces[i];
    if(target_muscle_force * deformation_rate > muscle_power) {
      target_muscle_force = (muscle_power / deformation_rate) || 0;
    }


    double length_difference = current_length - radius;
    double force_quotient = (
      - length_difference * inner_stiffness
      - deformation_rate * inner_damping// + target_muscle_force
    ) / (current_length);

    acc[0] -= v[0] * force_quotient / inner_mass;
    acc[1] -= v[1] * force_quotient / inner_mass;
    acc[i * 2 + 2] += v[0] * force_quotient / outer_vertex_mass;
    acc[i * 2 + 3] += v[1] * force_quotient / outer_vertex_mass;
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


    double length_difference = current_length - arc_distance * radius;
    double force_quotient = (
      - length_difference * outer_stiffness
      - deformation_rate * outer_damping
    ) / (current_length);

    acc[i * 2 + 2] -= v[0] * force_quotient / outer_vertex_mass;
    acc[i * 2 + 3] -= v[1] * force_quotient / outer_vertex_mass;

    acc[j * 2 + 2] += v[0] * force_quotient / outer_vertex_mass;
    acc[j * 2 + 3] += v[1] * force_quotient / outer_vertex_mass;
  }
  for(int i = 0; i < n_sides; i++) {
    int j = (i + 1) % n_sides;
    double v[2] = {
      pos[j * 2 + 2] - pos[i * 2 + 2],
      pos[j * 2 + 3] - pos[i * 2 + 3],
    };

    double edge_length = hypot(v[0], v[1]);
    double normal_direction[2] = {
      v[1] / edge_length,
      -v[0] / edge_length
    };

    double endpoint_velocities[4] = {
      vel[i * 2 + 2], vel[i * 2 + 3],
      vel[j * 2 + 2], vel[j * 2 + 3],
    };

    double kinematic_pressures[2] = {
      (endpoint_velocities[0] * normal_direction[0] + endpoint_velocities[1] * normal_direction[1]) * hypot(endpoint_velocities[0], endpoint_velocities[1]) * drag_factor,
      (endpoint_velocities[2] * normal_direction[0] + endpoint_velocities[3] * normal_direction[1]) * hypot(endpoint_velocities[2], endpoint_velocities[3]) * drag_factor//i couldnt find the formuila for kinematic pressure so I made one up :)
    };


    double pressure_acc_factor = 1 / outer_vertex_mass * fluid_interaction_width;

    acc[i * 2 + 2] -= v[1] * (kinematic_pressures[0] + total_pressure) * pressure_acc_factor;
    acc[i * 2 + 3] += v[0] * (kinematic_pressures[0] + total_pressure) * pressure_acc_factor;

    acc[j * 2 + 2] -= v[1] * (kinematic_pressures[1] + total_pressure) * pressure_acc_factor;
    acc[j * 2 + 3] += v[0] * (kinematic_pressures[1] + total_pressure) * pressure_acc_factor;
  }
}
