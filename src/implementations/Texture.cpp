#include "../../utils/Texture.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

Texture::Texture(const std::string &filename) {
  this->filename = filename;
}

Texture::Texture() {
  this->filename = "";
}

void Texture::loadTexture(){
  if(filename.size() == 0) return;

  std::ifstream file(filename);
  std::ifstream colors_debug("colors_debug.ppm");

  if(!file.is_open()){
    std::cout << "There was an error loading the texture: " << filename << ".\n";
    return;
  }

  std::string line;
  // ppm type
  std::getline(file, line);
  // dimension line
  std::getline(file, line);
  std::istringstream dimensions(line);
  int w, h;
  dimensions >> w >> h;
  this->width = w;
  this->height = h;
  int color_limit;
  file >> color_limit;

  colors.resize(h, std::vector<std::tuple<int, int, int>>(w));

  printf("texture width is %li, texture height is %li\n", colors[0].size(), colors.size());
  for(int i = 0; i < h; i++){
    for(int j = 0; j < w; j++){
      
      int r, g, b;
      file >> r >> g >> b;
      colors[i][j] = std::make_tuple(r, g, b);
    }
  }

  file.close();
  std::cout << "texture loaded\n";
}
