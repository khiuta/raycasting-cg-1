#include "../../utils/Texture.hpp"
#include <fstream>
#include <iostream>

Texture::Texture(const std::string &filename) {
  this->filename = filename;
}

Texture::Texture() {
  this->filename = "";
}

void Texture::loadTexture(){
  if(filename.size() == 0) return;

  std::ifstream file(filename);
  std::cout << filename << "\n";
  
  if(!file.is_open()){
    std::cout << "There was an error loading the texture: " << filename << ".\n";
    return;
  }

  std::string line;
  // Px line
  std::getline(file, line);
  // dimension line
  std::getline(file, line);
  int w, h;
  int dim_control = 0; // 0 is width, 1 is height
  // reading the dimension line
  for(int i = 0; i < line.size(); i++){
    if(line[i] >= 48 && line[i] <= 57){
      std::string dimension;
      int k = i;
      while(line[k] >= 48 && line[k] <= 57){
        dimension += line[k];
        k++;
      }
      i += k - i;

      if(dim_control == 0) w = std::stoi(dimension);
      else if(dim_control == 1) h = std::stoi(dimension);

      dim_control++;
    }
  }
  std::cout << w << " " << h << "\n";

  std::vector<std::vector<std::vector<int>>> matrix(
    h, 
    std::vector<std::vector<int>>(
      w, 
      // 3 is the number of channels of color (r, g, b)
      std::vector<int>(3, 0) // initialize r, g and b to 0
    )
  );

  int row = 0, column = 0;
  while(std::getline(file, line)){
    int r, g, b;  
    int color_control = 0; // 0 for red, 1 for green and 2 for blue
    
    for(int i = 0; i < line.size(); i++){  
      if(line[i] >= 48 && line[i] <= 57){
        int k = i;
        std::string color;
        
        while(line[k] >= 48 && line[k] <= 57){
          color += line[k];
          k++;
        }
        
        if(color_control == 0) r = std::stoi(color);
        else if(color_control == 1) g = std::stoi(color);
        else if(color_control == 2) b = std::stoi(color);
        
        i += k - i;
        color_control++;
        if(color_control > 2) {
          color_control = 0;
          std::cout << column << "\n";
          matrix[row][column] = {r, g, b};
          column++;
        }
      }
    }
  }
  //std::cout << row << "\n";
  row++;
  column = 0;
  std::cout << "finished\n";
}