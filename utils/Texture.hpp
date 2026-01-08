#ifndef TEXTURE
#define TEXTURE
#include <vector>
#include <string>

class Texture {
  public:
    std::string filename;
    std::vector<std::vector<std::tuple<int, int, int>>> colors;
    int width, height;

    void loadTexture();

    Texture();
    Texture(const std::string &filename);
};

#endif
