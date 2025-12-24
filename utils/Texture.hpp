#ifndef TEXTURE
#define TEXTURE
#include <vector>
#include <string>

class Texture {
  public:
    std::string filename;
    std::vector<std::vector<std::vector<int>>> colors{
      0, 
      std::vector<std::vector<int>>{
        0, 
        // 3 is the number of channels of color (r, g, b)
        std::vector<int>(0, 0) // initialize r, g and b to 0
      }
    };

    void loadTexture();

    Texture();
    Texture(const std::string &filename);
};

#endif