#include "../../utils/Texture.hpp"
#include <iostream>

// Definição necessária para a biblioteca funcionar (apenas neste .cpp)
#define STB_IMAGE_IMPLEMENTATION
#include "../../utils/stb_image.h" // Certifique-se que o caminho está correto

Texture::Texture(const std::string &filename) {
  this->filename = filename;
}

Texture::Texture() {
  this->filename = "";
}

void Texture::loadTexture(){
  if(filename.size() == 0) return;

  // Carrega a imagem usando stb_image
  // forçamos 4 canais (RGBA) com o último parâmetro '4'
  int w, h, nrChannels;
  unsigned char *data = stbi_load(filename.c_str(), &w, &h, &nrChannels, 4);

  if (!data) {
    std::cout << "Failed to load texture: " << filename << "\n";
    return;
  }

  this->width = w;
  this->height = h;

  // Redimensiona o vetor
  colors.resize(h, std::vector<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>>(w));

  // Copia os dados do stb_image para a nossa estrutura
  // O stb_image entrega um array linear, precisamos converter para matriz
  for(int i = 0; i < h; i++){
    for(int j = 0; j < w; j++){
      // O índice no array linear é: (linha * largura + coluna) * canais
      int index = (i * w + j) * 4;

      uint8_t r = data[index];
      uint8_t g = data[index + 1];
      uint8_t b = data[index + 2];
      uint8_t a = data[index + 3]; // Canal Alfa

      // Nota: Algumas texturas carregam invertidas verticalmente.
      // Se sua textura ficar de cabeça para baixo, troque 'colors[i]' por 'colors[h - 1 - i]'
      colors[i][j] = std::make_tuple(r, g, b, a);
    }
  }

  // Libera a memória alocada pelo stb_image
  stbi_image_free(data);
  
  std::cout << "Texture loaded: " << filename << " (" << w << "x" << h << ")\n";
}