#ifndef TEXTURE
#define TEXTURE
#include <vector>
#include <string>
#include <tuple>
#include <cstdint> 

class Texture {
  public:
    std::string filename;
  
    std::vector<std::vector<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>>> colors;
    int width, height;

    void loadTexture();

    Texture();
    Texture(const std::string &filename);
};

#endif