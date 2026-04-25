#include "../../utils/Texture.hpp"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "../../utils/stb_image.h"

Texture::Texture(const std::string &filename) { this->filename = filename; }

Texture::Texture() { this->filename = ""; }

void Texture::loadTexture() {
  if (filename.size() == 0)
    return;

  int w, h, nrChannels;
  unsigned char *data = stbi_load(filename.c_str(), &w, &h, &nrChannels, 4);

  if (!data) {
    std::cout << "Failed to load texture: " << filename << "\n";
    return;
  }

  this->width = w;
  this->height = h;

  colors.resize(h,
                std::vector<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>>(w));

  for (int i = 0; i < h; i++) {
    for (int j = 0; j < w; j++) {
      int index = (i * w + j) * 4;

      uint8_t r = data[index];
      uint8_t g = data[index + 1];
      uint8_t b = data[index + 2];
      uint8_t a = data[index + 3];

      colors[i][j] = std::make_tuple(r, g, b, a);
    }
  }

  stbi_image_free(data);

  std::cout << "Texture loaded: " << filename << " (" << w << "x" << h << ")\n";
}
