#ifndef TEXTURE
#define TEXTURE
#include <vector>
#include <string>

class Texture {
  public:
    std::string filename;
    std::vector<std::vector<int>> colors{0, std::vector<int>(0)};

    void loadTexture();

    Texture(const std::string &filename);
};

#endif