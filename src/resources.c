#include "./resources.h"
const unsigned char vert_basic_glsl[] = {
#embed "glsl/vert_basic.glsl"
};
const unsigned char vert_gazo_glsl[] = {
#embed "glsl/vert_gazo.glsl"
};
const unsigned char vert_3d_glsl[] = {
#embed "glsl/vert_3d.glsl"
};
const unsigned char vert_no_uv_map_glsl[] = {
#embed "glsl/vert_no_uv_map.glsl"
};
const unsigned char frag_basic_glsl[] = {
#embed "glsl/frag_basic.glsl"
};
const unsigned char frag_gamma_glsl[] = {
#embed "glsl/frag_gamma.glsl"
};
const unsigned char frag_gui_glsl[] = {
#embed "glsl/frag_gui.glsl"
};
const unsigned char gazo_spritesheet_png[] = {
#embed "../assets/textures/gazo_indexed.png"
}; const int gazo_spritesheet_png_len = sizeof(gazo_spritesheet_png) / sizeof(unsigned char);

const unsigned char stone_tile_png[] = {
#embed "../assets/textures/tiles.png"
}; const int stone_tile_png_len = sizeof(stone_tile_png) / sizeof(unsigned char);

const unsigned char bailey_truss_png[] = {
#embed "../assets/textures/bailey_truss_tilable.png"
}; const int bailey_truss_png_len = sizeof(bailey_truss_png) / sizeof(unsigned char);