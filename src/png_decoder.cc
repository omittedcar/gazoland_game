#include "png_decoder.h"
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void decode_png(const unsigned char png_data[], int png_size) {
  int width, height, channels;
  unsigned char* image_data = stbi_load_from_memory(
    png_data, png_size, &width, &height, &channels, 0
  );
  printf("channel: %i\n",channels);
  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_R8,
    width/4,
    height,
    0,
    GL_RED,
    GL_UNSIGNED_BYTE,
    image_data
  );
  stbi_image_free(image_data);
  
}
/*
static int png_pos;
static int png_len;
static const unsigned char* png_data;

void user_read_data(png_structp png_ptr, png_bytep data, png_size_t length) {
  if (length > png_len - png_pos) {
    length = png_len - png_pos;
  }
  std::memcpy(data, png_data + png_pos, length);
  png_pos += length;
}

void decode_png_with_pnglib(const unsigned char* png_data_, size_t len) {
  png_pos = 0;
  png_len = len;
  png_data = png_data_;
  png_structp png;
  // init
  png_set_read_fn(png, nullptr, &user_read_data);
}
*/