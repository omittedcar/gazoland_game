let is_float16 = confirm("For the color buffer, uint8 or float16?");
let is_physics_test = false;
let the_canvas = document.getElementById("the_canvas");
let video_stream = the_canvas.captureStream(60);
let video_recorder = new MediaRecorder(video_stream);
let recording_chunks = [];
let is_recording = false;
let gl = the_canvas.getContext("webgl2", {antialias: false, powerPreference: "high-performance"});
if (gl == null) {
  confirm("This is your computer 🖳");
}
let float16_color_ext = gl.getExtension("EXT_color_buffer_half_float");
gl.getExtension("EXT_color_buffer_float");
gl.getExtension("EXT_float_blend");
gl.getExtension("OES_texture_float_linear");

let current_music = new Audio("https://cdn.glitch.global/c5c3616a-4299-4f60-b0ff-508c52b79539/glebum%20title%20music%20wip.opus?v=1720563362110");
current_music.loop = true;
current_music.volume = 0.0625;




let is_game_is_it_playing = false;

let pre_motion_blur_texture = gl.createTexture();
let pre_motion_blur_depth_texture = gl.createTexture();
let post_motion_blur_texture = gl.createTexture();

let pre_motion_blur_framebuffer = gl.createFramebuffer();
let post_motion_blur_framebuffer = gl.createFramebuffer();

let vertical_resolution = 0;
let horizontal_resolution = 0;

function resize_textures(width, height) {
  gl.bindTexture(gl.TEXTURE_2D, pre_motion_blur_texture);
  
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);  
  if(is_float16){
    gl.texImage2D(gl.TEXTURE_2D, 0, float16_color_ext.RGBA16F_EXT, width, height, 0, gl.RGBA, gl.HALF_FLOAT, null);
  } else {
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
  }
  
  gl.bindFramebuffer(gl.FRAMEBUFFER, pre_motion_blur_framebuffer);
  gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, pre_motion_blur_texture, 0);
  
  gl.bindTexture(gl.TEXTURE_2D, pre_motion_blur_depth_texture);
  
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);  
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.DEPTH_COMPONENT24, width, height, 0, gl.DEPTH_COMPONENT, gl.UNSIGNED_INT, null);
  
  gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.DEPTH_ATTACHMENT, gl.TEXTURE_2D, pre_motion_blur_depth_texture, 0);
  
  gl.bindTexture(gl.TEXTURE_2D, post_motion_blur_texture);
  
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR); 
  if(is_float16) {
    gl.texImage2D(gl.TEXTURE_2D, 0, float16_color_ext.RGBA16F_EXT, width, height, 0, gl.RGBA, gl.HALF_FLOAT, null);
  } else {
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
  }
  
  gl.bindFramebuffer(gl.FRAMEBUFFER, post_motion_blur_framebuffer);
  gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, post_motion_blur_texture, 0);
}

resize_textures(4000, 4000);

let gembo_texture;
let glaggleland_texture;
let gazo_hand_texture;
let dither_texture;
let glaggle_vertex_shader_source = `
  attribute vec4 position_attribute;
  attribute vec2 uv_attribute;
  uniform mat4 view_matrix;
  uniform mat4 projection_matrix;
  uniform mat4 transform_matrix;
  varying vec2 uv;
  varying vec4 position;
  varying vec4 relative_position;
  void main() {
    position = transform_matrix * position_attribute;
    relative_position = view_matrix * position;
    uv = vec2(uv_attribute.x, -uv_attribute.y) * 0.5 + vec2(0.5);
    gl_Position = projection_matrix * relative_position;
  }
`;

let glaggle_fragment_shader_source = `
  precision highp float;
  uniform sampler2D color_sampler;
  uniform vec3 inputs;
  varying vec2 uv;
  varying vec4 position;
  varying vec4 relative_position;
  
  vec3 square_components (vec3 v) {
    return vec3(v.x * v.x, v.y * v.y, v.z * v.z);
  }
  
  void main() {
    vec4 texture_value = texture2D(color_sampler, uv * 1.0);
    vec3 surface_color = square_components(vec3(texture_value));
    vec3 fog_color = vec3(0.5, 0.5, 0.5);
    float clarity = pow(0.9375, length(relative_position));
    vec3 actual_color = surface_color * clarity + fog_color * (1.0 - clarity);
    gl_FragColor = vec4(actual_color, floor(texture_value.w + 0.5));
  }
`;

let glaggleland_vertex_shader_source = `
  precision highp float;
  attribute vec4 position_attribute; //🧊
  attribute vec3 normal_attribute;
  attribute vec2 uv_attribute;
  uniform mat4 view_matrix;
  uniform mat4 projection_matrix;
  uniform mat4 transform_matrix;
  varying vec4 position;
  varying vec4 relative_position;
  varying vec3 normal;
  varying vec2 uv;
  void main() {
    uv = uv_attribute;
    normal = normal_attribute;
    position = transform_matrix * position_attribute;
    relative_position = view_matrix * position;
    //vec4 projected_position = projection_matrix * relative_position;
    gl_Position = projection_matrix * relative_position;
    //gl_Position = vec4(projected_position.xyz / projected_position.w, 1.0);
  }
`;

let glaggleland_fragment_shader_source = `
  precision highp float;
  varying vec4 position;
  varying vec4 relative_position;
  varying vec3 normal;
  varying vec2 uv;
  uniform sampler2D color_sampler;
  
  vec3 square_components (vec3 v) {
    return vec3(v.x * v.x, v.y * v.y, v.z * v.z);
  }
  
  void main() {
    //vec3 n = normalize(normal);
    vec4 texture_value = texture2D(color_sampler, uv * 1.0);
    vec3 surface_color = square_components(vec3(texture_value));
    
    //float illumination = (abs(n.z) * 0.5 + n.y + 1.0) * 0.5 + 0.25;
    
    //vec3 surface_color_shaded = surface_color * illumination;
    vec3 fog_color = vec3(0.5, 0.5, 0.5);
    float clarity = pow(0.9375, length(relative_position));
    vec3 actual_color = surface_color * clarity + fog_color * (1.0 - clarity);
    gl_FragColor = vec4(actual_color, 1.0);
  }
`;

let basic_vertex_shader_source = `#version 300 es
in vec2 vertex_position;
out vec2 position;

void main() {
  position = vertex_position;
  gl_Position = vec4(position.x, position.y, 0.0, 1.0);
}
`;

let motion_blur_layer_shader_source = `#version 300 es
precision highp float;
uniform sampler2D color_sampler;
uniform float layer_opacity;
out vec4 pixel_color;
void main() {
  vec4 texel = texelFetch(color_sampler, ivec2(gl_FragCoord.x, gl_FragCoord.y), 0);
  pixel_color = vec4(texel.x, texel.y, texel.z, texel.w * layer_opacity);
}
`;

let downsample_and_gamma_shader_source = `#version 300 es
precision highp float;
in vec2 position;
uniform float inverse_screen_gamma;
uniform sampler2D color_sampler;
uniform ivec2 screen_dimensions;
out vec4 pixel_color;


void main() {
  ivec2 native_pixel_coord = ivec2(gl_FragCoord);
  ivec2 texture_dimensions = textureSize(color_sampler, 0);
  vec2 pixel_ratio = vec2(screen_dimensions) / vec2(texture_dimensions);
  
  ivec2 texel = ivec2(vec2(native_pixel_coord) / pixel_ratio);
  vec2 texel_offset = vec2(native_pixel_coord) - vec2(texel) * pixel_ratio;
  vec2 mixing_fac = min(texel_offset, vec2(1.0));
  //vec2 mixing_fac = vec2((texel_offset/pixel_ratio).x,min(texel_offset.y, 1.0));
  

  vec3 color_top = mix(
    vec3(texelFetch(
      color_sampler,
      texel - ivec2(1, 0),
      0
    )),
    vec3(texelFetch(
      color_sampler,
      texel - ivec2(0, 0),
      0
    )),
    mixing_fac.x
  );
  
  vec3 color_bottom = mix(
    vec3(texelFetch(
      color_sampler,
      texel - ivec2(1, 1),
      0
    )),
    vec3(texelFetch(
      color_sampler,
      texel - ivec2(0, 1),
      0
    )),
    mixing_fac.x
  );
  
  
  vec3 color_linear = mix(
    color_bottom,
    color_top,
    mixing_fac.y
  );
  


  
  pixel_color = vec4(

    pow(color_linear.x, inverse_screen_gamma),
    pow(color_linear.y, inverse_screen_gamma),
    pow(color_linear.z, inverse_screen_gamma),
    1.0
  );
  
  
}
`;

async function load_texture(url, gl) {
  let image = new Image();
  image.crossOrigin = "anonymous";
  image.src = url;
  let texture = gl.createTexture();
  
  gl.bindTexture(gl.TEXTURE_2D, texture);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);  
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, 1, 1, 0, gl.RGBA, gl.UNSIGNED_BYTE, new Uint8Array([255, 0, 255, 255]));
  
  await new Promise(function(resolve, reject) {
    this.onload = resolve;
  }.bind(image));
  gl.bindTexture(gl.TEXTURE_2D, texture);
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, image);
  //gl.generateMipmap(gl.TEXTURE_2D);
  return texture;
}

function load_model(obj_text, gl) {
  let text_lines = obj_text.split("\n");
  
  let n_text_lines = text_lines.length;
  
  
  let vertex_positions = [];
  let vertex_normals = [];
  let vertex_uv = [];
  
  let vertex_count = 0;
  let face_positions = [];
  let face_normals = [];
  let face_uv = [];
  
  for(let i = 0; i < n_text_lines; i++) {
    let line_here = text_lines[i];
    if(line_here.startsWith("v ")) {
      let words = line_here.split(" ");
      vertex_positions.push(
        parseFloat(words[1]),
        parseFloat(words[2]),
        parseFloat(words[3])
      );
      vertex_count++;
    }
    if(line_here.startsWith("vn")) {
      let words = line_here.split(" ");
      vertex_normals.push(
        parseFloat(words[1]),
        parseFloat(words[2]),
        parseFloat(words[3])
      );
    }
    if(line_here.startsWith("vt")) {
      let words = line_here.split(" ");
      vertex_uv.push(
        parseFloat(words[1]),
        parseFloat(words[2])
      );
    }
  }
  
  for(let i = 0; i < n_text_lines; i++) {
    let line_here = text_lines[i];
    if(line_here.startsWith("f")) {
      
      let words = line_here.split(" ");
      let corner_indexes = [                     // using irregualr verbs is bad form and stuff
        parseFloat(words[1].split("/")[0] - 1),
        parseFloat(words[2].split("/")[0] - 1),
        parseFloat(words[3].split("/")[0] - 1)
      ];
      
      let normal_indices = [
        parseFloat(words[1].split("/")[2] - 1),
        parseFloat(words[2].split("/")[2] - 1), //this code is poorly optimiszed and idc
        parseFloat(words[3].split("/")[2] - 1)
      ];
      
      let uv_indii = [
        parseFloat(words[1].split("/")[1] - 1),
        parseFloat(words[2].split("/")[1] - 1),
        parseFloat(words[3].split("/")[1] - 1)
      ];
      
      for(let j = 0; j < 3; j++) {
        let corner_index = corner_indexes[j];
        let normal_index = normal_indices[j];
        let uv_index = uv_indii[j];
        face_positions.push(
          vertex_positions[corner_index * 3],
          vertex_positions[corner_index * 3 + 1],
          vertex_positions[corner_index * 3 + 2]
        );
        face_normals.push(
          vertex_normals[normal_index * 3],
          vertex_normals[normal_index * 3 + 1],
          vertex_normals[normal_index * 3 + 2]
        );
        face_uv.push(
          vertex_uv[uv_index * 2],
          vertex_uv[uv_index * 2 + 1],
        );
      }
    }
  }

  return {
    position_buffer: initialise_vertex_buffer(face_positions, gl),
    normal_buffer: initialise_vertex_buffer(face_normals, gl), // it's a normal buffer
    uv_buffer: initialise_vertex_buffer(face_uv, gl)
  };
}

async function load_model_from_url(url, gl) {
  let fetch_response = await fetch(url);
  let obj_text = await fetch_response.text();
  return load_model(obj_text, gl);
}

function load_shader(gl, type, source) {
  let shader = gl.createShader(type);
  gl.shaderSource(shader, source);
  gl.compileShader(shader);
  if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
    alert(
      `We coudl not compile the shader😭😭😭😭😭😭😭${gl.getShaderInfoLog(shader)}`,
    );
    gl.deleteShader(shader);
    return null;
  }
  return shader;
}

function initialize_shader_program(gl, vertex_shader_source, fragment_shader_source) {
  let vertex_shader = load_shader(gl, gl.VERTEX_SHADER, vertex_shader_source);
  let fragment_shader = load_shader(gl, gl.FRAGMENT_SHADER, fragment_shader_source);
  let shader_program = gl.createProgram();
  gl.attachShader(shader_program, vertex_shader);
  gl.attachShader(shader_program, fragment_shader);
  gl.linkProgram(shader_program);
  if(!gl.getProgramParameter(shader_program, gl.LINK_STATUS)) {
    alert(`
      shader program did not waork because ${gl.getProgramInfoLog(shader_program)}
    `);
    return null
  }
  return shader_program;
}

let glaggle_shader_program = initialize_shader_program(gl, glaggle_vertex_shader_source, glaggle_fragment_shader_source);

let glaggle_program_info = {
  program: glaggle_shader_program,
  attribute_locations: {
    position: gl.getAttribLocation(glaggle_shader_program, "position_attribute"),
    uv: gl.getAttribLocation(glaggle_shader_program, "uv_attribute")
  },
  uniform_locations: {
    view_matrix: gl.getUniformLocation(glaggle_shader_program, "view_matrix"),
    projection_matrix: gl.getUniformLocation(glaggle_shader_program, "projection_matrix"),
    transform_matrix: gl.getUniformLocation(glaggle_shader_program, "transform_matrix"),
    color_sampler: gl.getUniformLocation(glaggle_shader_program, "color_sampler"),
  }
};

let glaggleland_shader_program = initialize_shader_program(gl, glaggleland_vertex_shader_source, glaggleland_fragment_shader_source);

let glaggleland_program_info = {
  program: glaggleland_shader_program,
  attribute_locations: {
    position: gl.getAttribLocation(glaggleland_shader_program, "position_attribute"),
    uv: gl.getAttribLocation(glaggleland_shader_program, "uv_attribute"),
    normal: gl.getAttribLocation(glaggleland_shader_program, "normal_attribute"),
  },
  uniform_locations: {
    view_matrix: gl.getUniformLocation(glaggleland_shader_program, "view_matrix"),
    projection_matrix: gl.getUniformLocation(glaggleland_shader_program, "projection_matrix"),
    transform_matrix: gl.getUniformLocation(glaggleland_shader_program, "transform_matrix"),
    color_sampler: gl.getUniformLocation(glaggleland_shader_program, "color_sampler")
  }
};

let motion_blur_layer_shader_program = initialize_shader_program(gl, basic_vertex_shader_source, motion_blur_layer_shader_source);
let motion_blur_layer_program_info = {
  program: motion_blur_layer_shader_program,
  position_attribute_location: gl.getAttribLocation(motion_blur_layer_shader_program, "vertex_position"),
  color_sampler_location: gl.getUniformLocation(motion_blur_layer_shader_program, "color_sampler"),
  layer_opacity_location: gl.getUniformLocation(motion_blur_layer_shader_program, "layer_opacity")
};

let downsample_and_gamma_shader_program = initialize_shader_program(gl, basic_vertex_shader_source, downsample_and_gamma_shader_source);
let downsample_and_gamma_program_info = {
  program: downsample_and_gamma_shader_program,
  position_attribute_location: gl.getAttribLocation(downsample_and_gamma_shader_program, "vertex_position"),
  color_sampler_location: gl.getUniformLocation(downsample_and_gamma_shader_program, "color_sampler"),
  inverse_screen_gamma_location: gl.getUniformLocation(downsample_and_gamma_shader_program, "inverse_screen_gamma"),
  screen_dimensions_location: gl.getUniformLocation(downsample_and_gamma_shader_program, "screen_dimensions")
};



let n_vertices = 16;
let n_outer_vertices = n_vertices - 1;

let the_angle_between_stuff = Math.PI * 2 / n_outer_vertices; 
let arc_distance = Math.hypot(Math.cos(the_angle_between_stuff) - 1, Math.sin(the_angle_between_stuff));


let uv_coords = [
  0, 0
];

for(let i = 0; i < n_outer_vertices; i++) {
  uv_coords.push(Math.cos(i * the_angle_between_stuff));
  uv_coords.push(Math.sin(i * the_angle_between_stuff));
}


let glagle_springs = [
];

for(let i = 0; i < n_outer_vertices; i++) {
  glagle_springs.push(
    0, i + 1,
    i + 1, (i + 1) % n_outer_vertices + 1
  );
}

let exposed_glaggle_edges = [
];

for(let i = 0; i < n_outer_vertices; i++) {
  exposed_glaggle_edges.push(
    i + 1, (i + 1) % n_outer_vertices + 1
  );
}

let glagggle_faces = [
];

for(let i = 0; i < n_outer_vertices; i++) {
  glagggle_faces.push(
    0, i + 1, (i + 1) % n_outer_vertices + 1
  );
}

function initialise_gazo_position_buffer(gl) {
  let position_buffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, position_buffer);
  
  let positions = [
    0.0, 1.0,
    Math.sqrt(3)/2, -0.5,
    Math.sqrt(3)/-2, -0.5
  ];
  
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(positions), gl.DYNAMIC_DRAW);
  return position_buffer;
}

function initialise_glaggle_uv_buffer(gl) {
  let uv_buffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, uv_buffer);
  
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(uv_coords), gl.STATIC_DRAW);
  return uv_buffer;
}

function initialise_glaggle_face_buffer(gl) {
  let face_buffer = gl.createBuffer();
  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, face_buffer);
  
  gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(glagggle_faces), gl.STATIC_DRAW);
  return face_buffer;
}

function initialise_vertex_buffer(vertex_array, gl) {
  let vertex_buffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, vertex_buffer);
  
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertex_array), gl.STATIC_DRAW);
  return vertex_buffer;
}

function initialise_element_buffer(element_array, gl) {
  let element_buffer = gl.createBuffer();
  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, element_buffer);
  
  gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(element_array), gl.STATIC_DRAW);
  return element_buffer;
}


let gazo_position_buffer = initialise_gazo_position_buffer(gl);
let glaggle_uv_buffer = initialise_glaggle_uv_buffer(gl);
let glaggle_face_buffer = initialise_glaggle_face_buffer(gl);

let glaggleland_position_buffer;
let glaggleland_normal_buffer;
let glaggleland_uv_buffer;

let gazo_hand_position_buffer;
let gazo_hand_uv_buffer;
let gazo_hand_normal_buffer;

let square_vertex_buffer = initialise_vertex_buffer([-1, -1, -1, 1, 1, -1, -1, 1, 1, -1, 1, 1], gl);




function get_glaggle_center_of_mass(vertex_positions) {
  let center_of_mass = [0, 0];
  for(let i = 0; i < n_vertices; i++) {
    for(let j = 0; j < 2; j++) {
      center_of_mass[j] += vertex_positions[i * 2 + j] * (i == 0 ? inner_mass : outer_vertex_mass);
    }
  }
  for(let i = 0; i < 2; i++) {
    center_of_mass[i] /= inner_mass + outer_mass;
  }
  return center_of_mass;
}

function get_glaggle_velocity(vertex_velocitys) {
  let velocity = [0, 0];
  for(let i = 0; i < n_vertices; i++) {
    for(let j = 0; j < 2; j++) {
      velocity[j] += vertex_velocitys[i * 2 + j] * (i == 0 ? inner_mass : outer_vertex_mass);
    }
  }
  for(let i = 0; i < 2; i++) {
    velocity[i] /= inner_mass + outer_mass;
  }
  return velocity;
}

class polygon {
  constructor(p) {
    this.points = p;
    this.n_points = p.length / 2;
    this.transform_matrix = [1, 0, 0, 1, 0, 0];
    this.velocity = [0, 0, 0, 0, 0, 0];
  }
  
  is_colliding(p) {
    let is_colliding = false;
    for(let i = 0; i < this.n_points; i++) {
      let j = (i + 1) % this.n_points;
      if(p[1] > Math.min(this.points[i * 2 + 1], this.points[j * 2 + 1])) {
        if(p[1] <= Math.max(this.points[i * 2 + 1], this.points[j * 2 + 1])) {
          if(p[0] <= Math.max(this.points[i * 2], this.points[j * 2])) {
            let x_intercept = (p[1] - this.points[i * 2 + 1]) * (this.points[j * 2] - this.points[i * 2]) / (this.points[j * 2 + 1] - this.points[i * 2 + 1]) + this.points[i * 2]
            if(this.points[i*2] == this.points[j*2] || p[0] <= x_intercept) {
              is_colliding = !is_colliding;
            }
          }
        }
      }
    }
    return is_colliding;
  }
  
  shortest_path(p0) {
    let path = [0,0];
    let distance_2 = Infinity;
    for(let i = 0; i < this.n_points; i++) {
      let j = (i + 1) % this.n_points;
      let p1 = [this.points[i * 2], this.points[i * 2 + 1]];
      let p2 = [this.points[j * 2], this.points[j * 2 + 1]];
      let atob = [p2[0] - p1[0], p2[1] - p1[1]];
      let atop = [p0[0] - p1[0], p0[1] - p1[1]];
      let len = atob[0] * atob[0] + atob[1] * atob[1];
      let dot = (atop[0] * atob[0]) + (atop[1] * atob[1]);
      let t = Math.min( 1, Math.max( 0, dot / len ) );
      let canidate = [p1[0] + atob[0] * t - p0[0],p1[1] + atob[1] * t - p0[1]];
      let candidate_distance_2 = canidate[0] * canidate[0] + canidate[1] * canidate[1];
      if(candidate_distance_2 < distance_2) {
        distance_2 = candidate_distance_2;
        path[0] = canidate[0];
        path[1] = canidate[1];
      }
    }
    return path;
  }
}

let current_level;
let levels = {};



let the_polygon = new polygon([
  -5.0, -0.1,
  -4.9, +0.0,
  +4.0, +0.0,
  +4.0, +15.0,
  -4.0, +15.0,
  -4.0, +13.0,
  +1.9, +13.0,
  +2.0, +12.9,
  +2.0, +1.6,
  +1.9, +1.5,
  +1.6, +1.5,
  +1.5, +1.6,
  +1.5, +12.5,
  -4.4, +12.5,
  -4.5, +12.6,
  -4.5, +15.4,
  -4.4, +15.5,
  +4.4, +15.5,
  +4.5, +15.4,
  +4.5, -0.4,
  +4.4, -0.5,
  -4.5, -0.5,
  -4.5, -1.9,
  -4.6, -2.0,
  
  -9.4, -2.0,
  -9.5, -1.9,
  -9.5, -0.1,
  -9.4, +0.0,
  -8.1, +0.0,
  -8.0, -0.1,
  -8.0, -1.5,
  -5.0, -1.5,
]);

class level {
  constructor(collision_shape, vertex_position_buffer, vertex_normal_buffer, texture_coordinate_buffer, texture) {
    this.collision_shape = collision_shape;
    this.vertex_position_buffer = vertex_position_buffer;
    this.vertex_normal_buffer = vertex_normal_buffer;
    this.texture_coordinate_buffer = texture_coordinate_buffer;
    this.texture = texture;
  }
}

function draw_scene_layer(layer_opacity) {
  
  gl.bindFramebuffer(gl.FRAMEBUFFER, pre_motion_blur_framebuffer);
  gl.viewport(0, 0, horizontal_resolution, vertical_resolution);
  
  gl.clearColor(0.5, 0.5, 0.5, 1.0);
  gl.clearDepth(1.0);
  gl.enable(gl.DEPTH_TEST);
  gl.depthFunc(gl.LEQUAL);
  gl.enable(gl.BLEND);
  gl.blendFunc(gl.SRC_COLOR, gl.DST_COLOR);
  
  
  gl.enable(gl.BLEND);
  gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
  
  gl.enable(gl.CULL_FACE);
  gl.cullFace(gl.BACK);
  
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  
  
  let aspect_ratio = the_canvas.width / the_canvas.height;
  let fov = 1.0;
  let near = 0.001;
  let far = null;
  
  let projection_matrix = [
    1 / aspect_ratio, 0, 0, 0,
    0, 1, 0, 0,
    (Math.random() * 2 - 1) / horizontal_resolution, (Math.random() * 2 - 1) / vertical_resolution, fov, fov,
    0, 0, -1, 0
  ];
  
  let view_matrix = [
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    -camera.position[0], -camera.position[1], viewing_distance, 1
  ];
  
  
  
  let transform_matrix = [
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
  ];
  
  
  
  
  
  //let floor_coords = new Float32Array(the_polygon.points);
  
  //gl.bindBuffer(gl.ARRAY_BUFFER, gazo_position_buffer);
  //gl.bufferData(gl.ARRAY_BUFFER, floor_coords, gl.DYNAMIC_DRAW);
  //gl.vertexAttribPointer(glaggle_program_info.attribute_locations.position, 2, gl.FLOAT, false, 0, 0);
  //gl.enableVertexAttribArray(glaggle_program_info.attribute_locations.position);

  //gl.bindBuffer(gl.ARRAY_BUFFER, glaggle_uv_buffer);
  //gl.bufferData(gl.ARRAY_BUFFER, floor_coords, gl.DYNAMIC_DRAW);
  //gl.vertexAttribPointer(glaggle_program_info.attribute_locations.uv, 2, gl.FLOAT, false, 0, 0);
  //gl.enableVertexAttribArray(glaggle_program_info.attribute_locations.uv);
  
  //gl.drawArrays(gl.LINE_LOOP, 0, the_polygon.n_points);
  
  //gl.disable(gl.CULL_FACE);
  
  gl.cullFace(gl.FRONT);
  
  gl.useProgram(glaggleland_shader_program);

  gl.uniformMatrix4fv(glaggleland_program_info.uniform_locations.view_matrix, false, view_matrix);
  gl.uniformMatrix4fv(glaggleland_program_info.uniform_locations.projection_matrix, false, projection_matrix);
  gl.uniformMatrix4fv(glaggleland_program_info.uniform_locations.transform_matrix, false, [
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
  ]);
  
  gl.bindBuffer(gl.ARRAY_BUFFER, current_level.vertex_position_buffer);
  gl.vertexAttribPointer(glaggleland_program_info.attribute_locations.position, 3, gl.FLOAT, false, 0, 0);
  gl.enableVertexAttribArray(glaggleland_program_info.attribute_locations.position);
  
  gl.bindBuffer(gl.ARRAY_BUFFER, current_level.vertex_normal_buffer);
  gl.vertexAttribPointer(glaggleland_program_info.attribute_locations.normal, 3, gl.FLOAT, false, 0, 0);
  gl.enableVertexAttribArray(glaggleland_program_info.attribute_locations.normal);
  
  gl.bindBuffer(gl.ARRAY_BUFFER, current_level.texture_coordinate_buffer);
  gl.vertexAttribPointer(glaggleland_program_info.attribute_locations.uv, 2, gl.FLOAT, false, 0, 0);
  gl.enableVertexAttribArray(glaggleland_program_info.attribute_locations.uv);
  
  gl.uniform1i(
  glaggleland_program_info.uniform_locations.color_sampler,
    0
  );
  
  gl.activeTexture(gl.TEXTURE0);
  gl.bindTexture(gl.TEXTURE_2D, current_level.texture);
  
  if(!is_physics_test) {
    gl.drawArrays(gl.TRIANGLES, 0, 9468);
  }
  
  gl.bindBuffer(gl.ARRAY_BUFFER, gazo_hand_position_buffer);
  gl.vertexAttribPointer(glaggleland_program_info.attribute_locations.position, 3, gl.FLOAT, false, 0, 0);
  gl.enableVertexAttribArray(glaggleland_program_info.attribute_locations.position);
  
  gl.bindBuffer(gl.ARRAY_BUFFER, gazo_hand_normal_buffer);
  gl.vertexAttribPointer(glaggleland_program_info.attribute_locations.normal, 3, gl.FLOAT, false, 0, 0);
  gl.enableVertexAttribArray(glaggleland_program_info.attribute_locations.normal);
  
  gl.bindBuffer(gl.ARRAY_BUFFER, gazo_hand_uv_buffer);
  gl.vertexAttribPointer(glaggleland_program_info.attribute_locations.uv, 2, gl.FLOAT, false, 0, 0);
  gl.enableVertexAttribArray(glaggleland_program_info.attribute_locations.uv);
  
  if(straining != 0) {
    
    gl.bindTexture(gl.TEXTURE_2D, gazo_hand_texture);
    
    let glaggle_velocity = get_glaggle_center_of_mass(glaggle.velocity);
    let glaggle_location = get_glaggle_center_of_mass(glaggle.position);
    let glaggle_inverse_speed = 1 / Math.hypot(...glaggle_velocity);
    let glaggle_direction = [
      glaggle_velocity[0] * glaggle_inverse_speed,
      glaggle_velocity[1] * glaggle_inverse_speed
    ]
    let wave = -Math.cos(misc_variables[1] * 0.5) + 1;
    let wave_b = Math.sin(misc_variables[1] * 0.25);
    
    gl.uniformMatrix4fv(glaggleland_program_info.uniform_locations.transform_matrix, false, [
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      glaggle_location[0] - glaggle_velocity[0] * (wave * 0.001) + glaggle_velocity[1] * (wave_b * 0.002) - glaggle_direction[0] * 0.5,
      glaggle_location[1] - glaggle_velocity[1] * (wave * 0.001) - glaggle_velocity[0] * (wave_b * 0.002) - glaggle_direction[1] * 0.5, 0, 1
    ]);
    
    gl.drawArrays(gl.TRIANGLES, 0, 180);
    
    
    
    gl.uniformMatrix4fv(glaggleland_program_info.uniform_locations.transform_matrix, false, [
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      glaggle_location[0] + glaggle_direction[0] * 0.3125 + glaggle_direction[1] * 0.25 * straining,
      glaggle_location[1] + glaggle_direction[1] * 0.3125 - glaggle_direction[0] * 0.25 * straining, 0, 1
    ]);
    
    gl.drawArrays(gl.TRIANGLES, 0, 180);
  }
  
  
  
  
  
  
  
  
  
  
  
  gl.cullFace(gl.BACK);
  let center_of_mass = get_glaggle_center_of_mass(glaggle.position);

  let triangle_vertex_positions = [
  ]
  
  for(let i = 0; i < n_vertices; i++) {
    let j = (i + 1) % n_vertices;
    triangle_vertex_positions.push(
      glaggle.position[i * 2], glaggle.position[i * 2 + 1],
      glaggle.position[j * 2], glaggle.position[j * 2 + 1],
      center_of_mass[0], center_of_mass[1]
    );
  }
  
  gl.bindBuffer(gl.ARRAY_BUFFER, gazo_position_buffer);
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(glaggle.position), gl.DYNAMIC_DRAW);
  gl.vertexAttribPointer(glaggle_program_info.attribute_locations.position, 2, gl.FLOAT, false, 0, 0);
  gl.enableVertexAttribArray(glaggle_program_info.attribute_locations.position);
  
  gl.bindBuffer(gl.ARRAY_BUFFER, glaggle_uv_buffer);
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(uv_coords), gl.DYNAMIC_DRAW);
  gl.vertexAttribPointer(glaggle_program_info.attribute_locations.uv, 2, gl.FLOAT, false, 0, 0);
  gl.enableVertexAttribArray(glaggle_program_info.attribute_locations.uv);
  
  gl.useProgram(glaggle_shader_program);
  
  gl.uniformMatrix4fv(glaggle_program_info.uniform_locations.view_matrix, false, view_matrix);
  gl.uniformMatrix4fv(glaggle_program_info.uniform_locations.projection_matrix, false, projection_matrix);
  gl.uniformMatrix4fv(glaggle_program_info.uniform_locations.transform_matrix, false, transform_matrix);
  
  gl.uniform1i(
  glaggle_program_info.uniform_locations.color_sampler,
    0
  );
  
  gl.activeTexture(gl.TEXTURE0);
  gl.bindTexture(gl.TEXTURE_2D, gembo_texture);
  
  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, glaggle_face_buffer);
  
  gl.drawElements(gl.TRIANGLES, glagggle_faces.length, gl.UNSIGNED_SHORT, 0);
  
  gl.disableVertexAttribArray(glaggle_program_info.attribute_locations.position);
  gl.disableVertexAttribArray(glaggle_program_info.attribute_locations.uv);
  
  
  
  
  
  
  
  
  
  gl.disableVertexAttribArray(glaggleland_program_info.attribute_locations.position);
  gl.disableVertexAttribArray(glaggleland_program_info.attribute_locations.uv);
  gl.disableVertexAttribArray(glaggleland_program_info.attribute_locations.normal);

  gl.bindFramebuffer(gl.FRAMEBUFFER, post_motion_blur_framebuffer);
  gl.viewport(0, 0, horizontal_resolution, vertical_resolution);
  gl.disable(gl.DEPTH_TEST);
  gl.disable(gl.CULL_FACE);
  gl.useProgram(motion_blur_layer_program_info.program);
  
  gl.bindBuffer(gl.ARRAY_BUFFER, square_vertex_buffer);
  gl.vertexAttribPointer(motion_blur_layer_program_info.position_attribute_location, 2, gl.FLOAT, false, 0, 0);
  gl.enableVertexAttribArray(motion_blur_layer_program_info.position_attribute_location);
  
  gl.uniform1f(
    motion_blur_layer_program_info.layer_opacity_location,
    layer_opacity
  );
  
  gl.uniform1i(
    motion_blur_layer_program_info.color_sampler_location,
    0
  );
  gl.activeTexture(gl.TEXTURE0);
  gl.bindTexture(gl.TEXTURE_2D, pre_motion_blur_texture);
  gl.drawArrays(gl.TRIANGLES, 0, 6);
  gl.disableVertexAttribArray(motion_blur_layer_program_info.position_attribute_location);
}

function display_scene() {
  

  gl.bindFramebuffer(gl.FRAMEBUFFER, null);
  gl.viewport(0, 0, the_canvas.width, the_canvas.height);
  
  gl.clearColor(0.0, 0.0, 0.0, 0.0);
  gl.clearDepth(1.0);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  
  gl.useProgram(downsample_and_gamma_program_info.program);
  
  gl.bindBuffer(gl.ARRAY_BUFFER, square_vertex_buffer);
  gl.vertexAttribPointer(downsample_and_gamma_program_info.position_attribute_location, 2, gl.FLOAT, false, 0, 0);
  gl.enableVertexAttribArray(downsample_and_gamma_program_info.position_attribute_location);
  
  gl.uniform1f(
    downsample_and_gamma_program_info.inverse_screen_gamma_location,
    0.5 // your screen's gamma is 0.5
  );
  
  gl.uniform1i(
    downsample_and_gamma_program_info.color_sampler_location,
    0
  );
  gl.activeTexture(gl.TEXTURE0);
  gl.bindTexture(gl.TEXTURE_2D, post_motion_blur_texture);
  
  gl.uniform2i(
    downsample_and_gamma_program_info.screen_dimensions_location,
    the_canvas.width,
    the_canvas.height
  );
  

  gl.drawArrays(gl.TRIANGLES, 0, 6);

  gl.disableVertexAttribArray(downsample_and_gamma_program_info.position_attribute_location);
  
  gl.bindFramebuffer(gl.FRAMEBUFFER, post_motion_blur_framebuffer);
  gl.clearColor(0.0, 0.0, 0.0, 0.0);
  gl.clearDepth(1.0);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
}



let frame_length = 1 / 60;
let steps = 7;
let step_length = 1 / 420;
let floor_height = -10;
let jump_height = 0;

let debug_info = document.getElementById("debug_info");

let pause_menu = document.getElementById("pause_menu");
let resume_button = document.getElementById("resume");
let exit_button = document.getElementById("exit");
let main_menu = document.getElementById("main_menu");
let record_button = document.getElementById("record");
let pleigh_button = document.getElementById("pleigh_button");
let recorded_video = document.getElementById("recorded_video");

let glaggle = {
  position: [
  ],
  velocity: [
  ]
};


let t = 0;
let resumed = false;
let pause_button = false;
let paralysed = false;
let resuming = false;
let can_pause_or_resume = false;
let is_480p_420_fps = true;

let current_gamepad;

let joystick_input = [0, 0];
let previous_joystick_b_input = [0, 0];
let joystick_b_input = [0, 0];
let wasd = [false, false, false, false];
let straining = 0.0; 

function copy_joystick_input() {
  let max_joystick_distance;
  if(current_gamepad) {
    max_joystick_distance = 1;
    
    joystick_input[0] = (current_gamepad.axes[0] * 1.25) || 0;
    joystick_input[1] = (-current_gamepad.axes[1] * 1.25) || 0;
    
    joystick_b_input[0] = current_gamepad.axes[5] || 0;
    joystick_b_input[1] = -current_gamepad.axes[2] || 0;
    straining = current_gamepad.buttons[5].value - current_gamepad.buttons[4].value;
  } else {
    max_joystick_distance = 1;
    joystick_input[0] = (cursor_position[0] - innerWidth / 2) / 50;
    joystick_input[1] = -(cursor_position[1] - innerHeight / 2) / 50;
    
    joystick_b_input[0] = (wasd[3] - wasd[1]) * max_joystick_distance;
    joystick_b_input[1] = (wasd[0] - wasd[2]) * max_joystick_distance;
    
  }
  let magnitude = Math.hypot(...joystick_input);
  if(magnitude > 1) {
    let rersfsbghjdk = 1 / magnitude;
    joystick_input[0] *= rersfsbghjdk;
    joystick_input[1] *= rersfsbghjdk;
  }
  
  let magnitude_b = Math.hypot(...joystick_b_input);
  if(magnitude_b > max_joystick_distance) {
    let rersfsbghjdk = max_joystick_distance / magnitude_b;
    joystick_b_input[0] *= rersfsbghjdk;
    joystick_b_input[1] *= rersfsbghjdk;
  }
}

function reset_joystick_input () {
  joystick_input[0] = 0;
  joystick_input[1] = 0;
  //joystick_b_input[0] = 0;
  //joystick_b_input[1] = 0;
}

let equilateral_triangle = [
  0, 1,
  Math.sqrt(3) / -2, -0.5,
  Math.sqrt(3) / 2, -0.5,
];


let glaggle_scale = 0.2; //the radius ☺️  f the glaggle

for(let i = 0; i < n_vertices; i++) {
  
  glaggle.position.push(
    uv_coords[i * 2] * glaggle_scale + 3,
    uv_coords[i * 2 + 1] * glaggle_scale + 13
  );
  
  glaggle.velocity.push(
    0, 0
  );
}




let camera = {
  position: [0, 0],
  velocity: [0, 0],
}

function advance_camera() {
  
  let target_pos = get_glaggle_center_of_mass(glaggle.position);
  
  
  camera.position[0] = target_pos[0];
  camera.position[1] = target_pos[1];
}




//  length is in fathoms and time is in seconds and mass is in okka
// 


//earth constants ↓

let air_pressure = 0x18BCD;
let gravity = 0x9CF / 256;


//arbitrary constants ↓

let fluid_interaction_width = 0.1;

let inner_mass = 0x8;
let outer_mass = 0xA;

let outer_stiffness = 0x1000;
let outer_damping = 0x20;

let muscle_power = 0x200; //each muscle gets a mildly impressive 512 watts. 

let inner_stiffness = 0x600;
let inner_damping = 0xA;


let internal_pressure = 0x3400;

let friction_coefficient = 1.0;

let viewing_distance = 0x3;

let drag_factor = 0x3;


let outer_vertex_mass = outer_mass / n_outer_vertices;

let dash_speed = 3.5;

let cursor_position = [0, 0];


function calc_glaggle_acc(system) {
  

  let acc = [];
  
  let glaggle_pos = system[0];
  let glaggle_vel = system[1];
  let glaggle_vel_avg = get_glaggle_center_of_mass(glaggle_vel);
  let glaggle_speed = Math.hypot(...glaggle_vel_avg);
  
  for(let i = 0; i < n_vertices * 2; i++) {
    acc.push(0);
  }
  
  let straining_force = [
    glaggle_vel_avg[1] * straining * glaggle_speed * .09375,
    -glaggle_vel_avg[0] * straining * glaggle_speed * .09375
  ]
  
  for(let i = 0; i < n_vertices; i++) { // ACERLTATION DUE TO GREAVITY
    acc[i * 2] += straining_force[0];
    acc[i * 2 + 1] += straining_force[1] - gravity;
  }
  
  let polygon_area = 0;
  
  for(let i = 0; i < n_outer_vertices; i++) {
    let j = (i + 1) % n_outer_vertices;
    polygon_area += glaggle_pos[i * 2 + 2] * glaggle_pos[j * 2 + 3] - glaggle_pos[j * 2 + 2] * glaggle_pos[i * 2 + 3]
  }
  
  polygon_area /= 2;
  let total_pressure = (-internal_pressure / polygon_area + air_pressure * Math.sign(polygon_area));
  
  
  let glaggle_rotation = [0, 0];
  let glaggle_center = [glaggle_pos[0], glaggle_pos[1]];
  
  for(let i = 0; i < n_outer_vertices; i++) {
    glaggle_rotation[0] += (glaggle_pos[i * 2 + 2] - glaggle_center[0]) * uv_coords[i * 2 + 2] + (glaggle_pos[i * 2 + 3] - glaggle_center[1]) * uv_coords[i * 2 + 3];
    glaggle_rotation[1] += (glaggle_pos[i * 2 + 2] - glaggle_center[0]) * uv_coords[i * 2 + 3] - (glaggle_pos[i * 2 + 3] - glaggle_center[1]) * uv_coords[i * 2 + 2];
  }
  
  let glaggle_rotation_normaliser = 1 / Math.hypot(...glaggle_rotation);
  glaggle_rotation[0] *= glaggle_rotation_normaliser;
  glaggle_rotation[1] *= glaggle_rotation_normaliser;
  
  let rotated_joystick = [
    is_physics_test?0:(-joystick_input[0] * glaggle_rotation[0] + joystick_input[1] * glaggle_rotation[1]),
    is_physics_test?0:(-joystick_input[1] * glaggle_rotation[0] - joystick_input[0] * glaggle_rotation[1])
  ];
  

  let muscle_forces = [];
  

  for(let i = 0; i < n_outer_vertices; i++) {
    muscle_forces.push(
      (Math.hypot(
        uv_coords[i * 2 + 2] + rotated_joystick[0] * 0.625,
        uv_coords[i * 2 + 3] + rotated_joystick[1] * 0.625
      ) - 1) * inner_stiffness * glaggle_scale * 1
    );
  }
  
  for(let i = 0; i < n_outer_vertices; i++) {
    let v = [
      glaggle_pos[i * 2 + 2] - glaggle_pos[0],
      glaggle_pos[i * 2 + 3] - glaggle_pos[1]
    ];
    let current_length = Math.hypot(...v);
    let deformation_rate = (
      (glaggle_vel[i * 2 + 2] - glaggle_vel[0]) * v[0] +
      (glaggle_vel[i * 2 + 3] - glaggle_vel[1]) * v[1]
    ) / current_length;
    
    //let is_outer_spring = i % 2 == 1;
    
    let target_muscle_force = muscle_forces[i];
    if(target_muscle_force * deformation_rate > muscle_power) {
      target_muscle_force = (muscle_power / deformation_rate) || 0;
    }

    //let target_muscle_force = 0;
    
    let length_difference = glaggle_scale - current_length;
    let acc_factor = (length_difference * inner_stiffness - deformation_rate * inner_damping + target_muscle_force) / (current_length);
    
    //let mass_a = (glagle_springs[i * 2] == 0) ? inner_mass : outer_vertex_mass;
    //let mass_b = (glagle_springs[i * 2 + 1] == 0) ? inner_mass : outer_vertex_mass;
    
    acc[0] -= v[0] * acc_factor / inner_mass;
    acc[1] -= v[1] * acc_factor / inner_mass;
    acc[i * 2 + 2] += v[0] * acc_factor / outer_vertex_mass;
    acc[i * 2 + 3] += v[1] * acc_factor / outer_vertex_mass;
  }
  
  for(let i = 0; i < n_outer_vertices; i++) {
    let v = [
      glaggle_pos[i * 2 + 2] - glaggle_pos[((i+1) % n_outer_vertices) * 2 + 2],
      glaggle_pos[i * 2 + 3] - glaggle_pos[((i+1) % n_outer_vertices) * 2 + 3]
    ];
    let current_length = Math.hypot(...v);
    let deformation_rate = (
      (glaggle_vel[i * 2 + 2] - glaggle_vel[((i+1) % n_outer_vertices) * 2 + 2]) * v[0] +
      (glaggle_vel[i * 2 + 3] - glaggle_vel[((i+1) % n_outer_vertices) * 2 + 3]) * v[1]
    ) / current_length;

    
    let length_difference = arc_distance * glaggle_scale - current_length;
    let acc_factor = (length_difference * outer_stiffness - deformation_rate * outer_damping) / (current_length); 
    
    

    acc[((i+1) % n_outer_vertices) * 2 + 2] -= v[0] * acc_factor / outer_vertex_mass;
    acc[((i+1) % n_outer_vertices) * 2 + 3] -= v[1] * acc_factor / outer_vertex_mass;
    acc[i * 2 + 2] += v[0] * acc_factor / outer_vertex_mass;
    acc[i * 2 + 3] += v[1] * acc_factor / outer_vertex_mass;
  }
  
  for(let i = 0; i < exposed_glaggle_edges.length / 2; i++) {
    
    let v = [
      glaggle_pos[exposed_glaggle_edges[i * 2 + 1] * 2] - glaggle_pos[exposed_glaggle_edges[i * 2] * 2],
      glaggle_pos[exposed_glaggle_edges[i * 2 + 1] * 2 + 1] - glaggle_pos[exposed_glaggle_edges[i * 2] * 2 + 1],
    ];
    
    let edge_length = Math.hypot(...v);
    let normal_direction = [
      v[1] / edge_length,
      -v[0] / edge_length
    ];
    
    let endpoint_velocities = [
      glaggle_vel[exposed_glaggle_edges[i * 2] * 2], glaggle_vel[exposed_glaggle_edges[i * 2] * 2 + 1],
      glaggle_vel[exposed_glaggle_edges[i * 2 + 1] * 2], glaggle_vel[exposed_glaggle_edges[i * 2 + 1] * 2 + 1]
    ];
    
    let kinematic_pressures = [
      (endpoint_velocities[0] * normal_direction[0] + endpoint_velocities[1] * normal_direction[1]) * Math.hypot(endpoint_velocities[0], endpoint_velocities[1]) * drag_factor,
      (endpoint_velocities[2] * normal_direction[0] + endpoint_velocities[3] * normal_direction[1]) * Math.hypot(endpoint_velocities[2], endpoint_velocities[3]) * drag_factor//i couldnt find the formuila for kinematic pressure so I made one up :)
    ];
    
    
    let pressure_acc_factor = 1 / outer_vertex_mass * fluid_interaction_width;
    
    acc[exposed_glaggle_edges[i * 2] * 2] -= v[1] * (kinematic_pressures[0] + total_pressure) * pressure_acc_factor;
    acc[exposed_glaggle_edges[i * 2] * 2 + 1] += v[0] * (kinematic_pressures[0] + total_pressure) * pressure_acc_factor;
    
    acc[exposed_glaggle_edges[i * 2 + 1] * 2] -= v[1] * (kinematic_pressures[1] + total_pressure) * pressure_acc_factor;
    acc[exposed_glaggle_edges[i * 2 + 1] * 2 + 1] += v[0] * (kinematic_pressures[1] + total_pressure) * pressure_acc_factor;
  }

  return acc;
}

function copy_velocity(system) {
  let velocity = system[1];
  let return_value = [];
  for(let i = 0; i < velocity.length; i++) {
    return_value.push(velocity[i]);
  }
  return return_value; //return the return value
}

let misc_variables = [0, 0];

let differential_equation_variables = [
  glaggle.position,
  glaggle.velocity,
  misc_variables
];

let differential_equation_derivatives = [
  copy_velocity,
  calc_glaggle_acc,
  function(system) {
    let glaggle_vel_avg = get_glaggle_center_of_mass(system[1])
    return [
      1,
      Math.hypot(...glaggle_vel_avg)**2
    ];
  }
];

function copy_array_of_arrays(array_to_copy) {
  let new_array = [];
  array_to_copy.forEach(other_array => {
    let copied_array = [];
    other_array.forEach(value => {
      copied_array.push(value);
    });
    new_array.push(copied_array);
  });
  return new_array;
}

function calculate_all_derivatives(variables, derivatives) {
  let all_derivatives = [];
  for(let i = 0; i < derivatives.length; i++) {
    all_derivatives.push(derivatives[i](variables));
  }
  return all_derivatives;
}


function average_derivatives(k0, k1, k2, k3) {
  let average_derivatives = [];
  
  let return_value = [];
  for(let i = 0; i < k0.length; i++) {
    let new_array = [];
    for(let j = 0; j < k0[i].length; j++) {
      new_array.push(k0[i][j]);
      new_array[j] += 2 * k1[i][j];
      new_array[j] += 2 * k2[i][j];
      new_array[j] += k3[i][j];
      new_array[j] /= 6;
    }
    return_value.push(new_array);
  }
  
  return return_value;
  
  return average_derivatives;
}

function advance_all_variables(variables, derivatives, h) {
  let return_value = [];
  for(let i = 0; i < variables.length; i++) {
    let new_array = [];
    for(let j = 0; j < variables[i].length; j++) {
      new_array.push(variables[i][j] + derivatives[i][j] * h);
    }
    return_value.push(new_array);
  }
  return return_value;
}

function copy_array(array_in, array_out) {
  for(let i = 0; i < array_in.length; i++) {
    let value_in = array_in[i];
    for(let j = 0; j < value_in.length; j++) {
      array_out[i][j] = array_in[i][j];
    }
  }
}

function evolve_system(variables, derivatives, time_step) { //using the Runge-Kutta method
  
  let initial_derivatives = calculate_all_derivatives(variables, derivatives);
  
  let midpoint_values_a = advance_all_variables(variables, initial_derivatives, time_step / 2);
  let midpoint_derivatives_a = calculate_all_derivatives(midpoint_values_a, derivatives);
  
  let midpoint_values_b = advance_all_variables(variables, midpoint_derivatives_a, time_step / 2);
  let midpoint_derivatives_b = calculate_all_derivatives(midpoint_values_b, derivatives);
  
  let endpoint_values = advance_all_variables(variables, midpoint_derivatives_b, time_step);
  let endpoint_derivatives = calculate_all_derivatives(endpoint_values, derivatives);
  
  let final_values = advance_all_variables(variables, average_derivatives(initial_derivatives, midpoint_derivatives_a, midpoint_derivatives_b, endpoint_derivatives), time_step);
  
  copy_array(final_values, variables);
}


function correct_collisions(time_step) {
  
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
}

function advance() {
  
  
  advance_camera();
  
  evolve_system(differential_equation_variables, differential_equation_derivatives, step_length);
  
  correct_collisions(step_length);
  

}

function apply_weird_force() {
  for(let i = 0; i < n_vertices; i++){
    glaggle.velocity[i * 2] += (joystick_b_input[0] - previous_joystick_b_input[0]) * dash_speed;
    glaggle.velocity[i * 2 + 1] += (joystick_b_input[1] - previous_joystick_b_input[1]) * dash_speed;
  }
}

function pause_or_resume() {
  if(can_pause_or_resume) {
    if(resumed) {
      pause();
    } else {
      resume();
    }
  }
}

function pause() {
  resumed = false;
  pause_menu.style.display = "flex";
}
function resume() {
  resumed = true;
  pause_menu.style.display = "none";
}

function un_paralyse() {
  paralysed = false;
}

function tick() {
  
  update_gamepads();
  if(!paralysed) {
    copy_joystick_input();
  } 
  
  if(current_gamepad) {
    if(current_gamepad.buttons[9].pressed && !pause_button) {
      pause_or_resume();
    }

    pause_button = current_gamepad.buttons[9].pressed;
  }
  
  if(resumed) {
    apply_weird_force();
  
    
    for(let i = 0; i < (is_physics_test ? 1 : steps) && resumed; i++) {
      advance();
      draw_scene_layer(1 / (i + 1));
      //gl.flush();
    }
    //draw_scene_layer(1);
    
    gl.viewport(0, 0, the_canvas.width, the_canvas.height);
    display_scene();
    
    previous_joystick_b_input[0] = joystick_b_input[0];
    previous_joystick_b_input[1] = joystick_b_input[1];
  }
  
  let velocity = get_glaggle_center_of_mass(glaggle.velocity);
  
  debug_info.innerHTML = `
    X: ${(velocity[0] >= 0 ? "+" : "") + velocity[0].toString(10)}<br>
    Y: ${(velocity[1] >= 0 ? "+" : "") + velocity[1].toString(10)}
  `;
  
  requestAnimationFrame(tick);
}



document.addEventListener("keydown", function(e) {
  if(!e.repeat) {
    if(e.key == "w") {
      wasd[0] = true;
    }
    if(e.key == "a") {
      wasd[1] = true;
    }
    if(e.key == "s") {
      wasd[2] = true;
    }
    if(e.key == "d") {
      wasd[3] = true;
    }
    if(e.key == "Escape") {
      pause_or_resume()
    }
    if(e.key == "q") {
      straining--;
      straining = Math.max(Math.min(straining, 1), -1);
    }
    if(e.key == "e") {
      straining++;
      straining = Math.max(Math.min(straining, 1), -1);
    }
  }
});

document.addEventListener("keyup", function(e) {
  if(!e.repeat) {
    if(e.key == "w") {
      wasd[0] = false;
    }
    if(e.key == "a") {
      wasd[1] = false;
    }
    if(e.key == "s") {
      wasd[2] = false;
    }
    if(e.key == "d") {
      wasd[3] = false;
    }
    if(e.key == "q") {
      straining++;
      straining = Math.max(Math.min(straining, 1), -1);
    }
    if(e.key == "e") {
      straining--;
      straining = Math.max(Math.min(straining, 1), -1);
    }
  }
});

function adjust_canvas() {
  the_canvas.width = window.innerWidth * window.devicePixelRatio;
  the_canvas.height = window.innerHeight * window.devicePixelRatio;
  horizontal_resolution = Math.round(window.innerWidth / window.innerHeight * 480);
  vertical_resolution = 480;
  //horizontal_resolution = the_canvas.width;
  //vertical_resolution = the_canvas.height;
  //the_canvas.width = Math.round(window.innerWidth / 2);
  //the_canvas.height = Math.round(window.innerHeight / 2);
  resize_textures(horizontal_resolution, vertical_resolution);
}

function exit_game() {
  current_music.load();
  current_music.play();
  can_pause_or_resume = false;
  resumed = false;
  pause_menu.style.display = "none";
  main_menu.style.display = "flex";
}
function play() {
  current_music.pause();
  can_pause_or_resume = true;
  resumed = true;
  main_menu.style.display = "none";
  previous_joystick_b_input[0] = 0;
  previous_joystick_b_input[1] = 0;
  for(let i = 0; i < n_vertices; i++) {
  
    glaggle.position[i * 2] = uv_coords[i * 2] * glaggle_scale;
    glaggle.position[i * 2 + 1] = uv_coords[i * 2 + 1] * glaggle_scale + 1;
    glaggle.velocity[i * 2] = -0x18;
    glaggle.velocity[i * 2 + 1] = 0;
  }
}
function show_main_menu() {
  main_menu.style.display = "flex";
}

window.addEventListener("resize", adjust_canvas);
window.addEventListener("load", adjust_canvas);

async function load_textures() {
  await Promise.all([
    load_texture("https://cdn.glitch.global/c5c3616a-4299-4f60-b0ff-508c52b79539/0dbced45-be5f-4951-a47d-9046c99b4d34.image.png?v=1717552311712", gl),
    load_texture("https://cdn.glitch.global/c5c3616a-4299-4f60-b0ff-508c52b79539/45164f91-3596-46c2-88ab-6031d6bcf44f.image.png?v=1717274925016", gl),
    load_texture("https://cdn.glitch.global/c5c3616a-4299-4f60-b0ff-508c52b79539/gazo_hand_texture.png?v=1723919607393", gl),
    load_texture("https://cdn.glitch.global/c5c3616a-4299-4f60-b0ff-508c52b79539/dither.png?v=1725490830744", gl),
  ]).then(textures => {
    gembo_texture = textures[0];
    glaggleland_texture = textures[1];
    gazo_hand_texture = textures[2];
    dither_texture = textures[3];
  });
}

async function load_models() {
  await Promise.all([
    load_model_from_url("https://cdn.glitch.global/c5c3616a-4299-4f60-b0ff-508c52b79539/gazo_test_level_uv%2Bnormals.obj?v=1716606375034", gl),
    load_model_from_url("https://cdn.glitch.global/c5c3616a-4299-4f60-b0ff-508c52b79539/gazo_hand.obj?v=1723918978274", gl),
    
  ]).then(models => {
    levels.test_level = new level(
      the_polygon,
      models[0].position_buffer,
      models[0].normal_buffer,
      models[0].uv_buffer,
      glaggleland_texture
    );
    gazo_hand_position_buffer = models[1].position_buffer;
    gazo_hand_uv_buffer = models[1].uv_buffer;
    gazo_hand_normal_buffer = models[1].normal_buffer;
  });
}

function digital_joystick_handler(e) {
  
}

function update_gamepads () {
  current_gamepad = navigator.getGamepads()[0] || null;
}


function record_button_handler() {
  if(is_recording) {
    video_recorder.stop();
    record_button.style.background = "";
  } else {
    video_recorder.start();
    record_button.style.background = "red";
  }
  is_recording = !is_recording;
}

video_recorder.ondataavailable = (e) => {
  recording_chunks.push(e.data);
}
video_recorder.onstop = (e) => {
  recorded_video.src = window.URL.createObjectURL(new Blob(recording_chunks, {type:"video/mp4;"}));
  recording_chunks = [];
}
// Startup code...

document.addEventListener("mousemove", function(e) {
  cursor_position[0] = e.clientX;
  cursor_position[1] = e.clientY;
});

document.addEventListener("gamepaddisconnected", update_gamepads);
document.addEventListener("gamepadconnected", update_gamepads);

resume_button.addEventListener("click", function() {
  if(can_pause_or_resume && !resumed) {
    resume();
  }
});


record_button.addEventListener("click", record_button_handler);

exit_button.addEventListener("click", exit_game);

async function initialize_game() {//merca🦅
  if(!is_game_is_it_playing) {
    is_game_is_it_playing = true;
    await load_textures();
    await load_models();
    current_level = levels.test_level;
    tick();
    pleigh_button.addEventListener("click", play);
  }
}

document.oncontextmenu = (e) => {e.preventDefault();}


initialize_game();
