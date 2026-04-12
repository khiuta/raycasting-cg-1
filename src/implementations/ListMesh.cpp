#include "../../utils/ListMesh.hpp"
#include "../../utils/BVH.hpp"
#include <iostream>
#include "../../utils/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
ListMesh::ListMesh() {};

ListMesh::ListMesh(const std::string &filename) {
  texture->filename = filename;
  texture->loadTexture();
};

ListMesh::ListMesh(std::vector<std::unique_ptr<Triangle>> faces, std::vector<std::unique_ptr<Point4>> vertices, Point4 centroid, AABB aabb)
  : faces(std::move(faces)), vertices(std::move(vertices)), centroid(centroid), aabb(std::move(aabb)) {

  };

bool ListMesh::Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const {
  return aabb.Hit(origin, dir, t_min, t_max, hr);
}

void ListMesh::rebuildStructures() {
    if (faces.empty()) return;

    // 1. REPOVOAR A LISTA DE TRIÂNGULOS DA AABB RAIZ
    this->aabb.t.clear();
    for(auto& face : faces) {
        this->aabb.t.push_back(face.get());
    }

    // 2. RECÁLCULO DA AABB GLOBAL DA MALHA
    float minX = 1e30f, minY = 1e30f, minZ = 1e30f;
    float maxX = -1e30f, maxY = -1e30f, maxZ = -1e30f;

    for (auto& t : faces) {
        // ATENÇÃO AQUI: Como p1, p2 e p3 agora são ponteiros, usamos '->'
        float xs[] = {t->p1->x, t->p2->x, t->p3->x};
        float ys[] = {t->p1->y, t->p2->y, t->p3->y};
        float zs[] = {t->p1->z, t->p2->z, t->p3->z};

        for(int i = 0; i < 3; i++) {
            if(xs[i] < minX) minX = xs[i];
            if(xs[i] > maxX) maxX = xs[i];
            if(ys[i] < minY) minY = ys[i];
            if(ys[i] > maxY) maxY = ys[i];
            if(zs[i] < minZ) minZ = zs[i];
            if(zs[i] > maxZ) maxZ = zs[i];
        }
    }

    this->aabb.min_x = minX; this->aabb.max_x = maxX;
    this->aabb.min_y = minY; this->aabb.max_y = maxY;
    this->aabb.min_z = minZ; this->aabb.max_z = maxZ;

    // 3. ATUALIZAÇÃO DO CENTRÓIDE
    this->centroid = Point4((minX + maxX) / 2.0f, (minY + maxY) / 2.0f, (minZ + maxZ) / 2.0f, 1.0f);

    // 4. RECONSTRUÇÃO DA HIERARQUIA (BVH)
    this->aabb.buildBVH(10); 
}

// =======================================================================
// NOVAS FUNÇÕES DE TRANSFORMAÇÃO: Atuam nos vértices originais!
// =======================================================================

void ListMesh::applyTranslate(const Matrix4 &m) {
    // 1. Aplica a matriz em todos os vértices base (Point4)
    for(auto& v : vertices) {
        Vector4 vec = m * Vector4(v->x, v->y, v->z, 1.0f);
        v->x = vec.x; v->y = vec.y; v->z = vec.z;
    }
    // 2. Avisa os triângulos para recalcularem normais e arestas
    for(auto& t : faces) t->recalculateProperties();
    // 3. Atualiza as caixas de colisão
    rebuildStructures();
}

void ListMesh::applyScale(const Matrix4 &m) {
    for(auto& v : vertices) {
        Vector4 vec = m * Vector4(v->x, v->y, v->z, 1.0f);
        v->x = vec.x; v->y = vec.y; v->z = vec.z;
    }
    for(auto& t : faces) t->recalculateProperties();
    rebuildStructures();
}

void ListMesh::applyRotation(const Matrix4 &m) {
    for(auto& v : vertices) {
        Vector4 vec = m * Vector4(v->x, v->y, v->z, 1.0f);
        v->x = vec.x; v->y = vec.y; v->z = vec.z;
    }
    for(auto& t : faces) t->recalculateProperties();
    rebuildStructures();
}

void ListMesh::applyShear(const Matrix4 &m) {
    for(auto& v : vertices) {
        Vector4 vec = m * Vector4(v->x, v->y, v->z, 1.0f);
        v->x = vec.x; v->y = vec.y; v->z = vec.z;
    }
    for(auto& t : faces) t->recalculateProperties();
    rebuildStructures();
}

// =======================================================================

std::vector<float> ListMesh::FlattenVertices() {
    std::vector<float> flatData;
    flatData.reserve(vertices.size() * 3); 

    for (const auto& pt : vertices) {
        flatData.push_back(pt->x);
        flatData.push_back(pt->y);
        flatData.push_back(pt->z);
    }
    
    return flatData;
}

static unsigned int compileShader(unsigned int type,
                                  const std::string &source) {
  // Lembra que para criar um buffer primeiro criamos um variável
  // para depois passá-la dentro da função glGenBuffers.
  // Mas para criar um shader, a função glCreateShader já
  // retorna o id do shader.
  // Pois é, fugiu um pouco do padrão que aprendemos. né?
  // Bem vindo ao OpenGL! <3
  unsigned int id = glCreateShader(type);

  const char *src =
      source.c_str(); // Vamos pegar nossa string c++ e transformar
                      // em um array de chars, típico do c.
  // Criamos o id do shader e agora vamos passar o código para ele
  glShaderSource(id, 1, &src, nullptr);
  glCompileShader(id);

  int result;
  // Essa função retorna um parâmetro do shader
  //  GL_SHADER_TYPE, GL_DELETE_STATUS, GL_COMPILE_STATUS, GL_INFO_LOG_LENGTH,
  //  GL_SHADER_SOURCE_LENGTH.
  glGetShaderiv(id, GL_COMPILE_STATUS, &result);
  // Result será 0 caso ocorra um erro e 1 caso não ocorra

  // Como o valor ou é 0 ou 1, podemos tratá-los como booleanos nesse if
  if (!result) {
    int lenght;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &lenght);
    char *message = (char *)alloca(lenght * sizeof(char));
    glGetShaderInfoLog(id, lenght, &lenght, message);
    std::cout << "FAILED TO COMPILE SHADER"
              << (type == GL_VERTEX_SHADER ? "Vertex Shader"
                                           : "Fragment Shader")
              << "\n";
    std::cout << message << "\n";
    glDeleteShader(id);
    return 0;
  }
  return id; // retorna o id  do shader
}
static unsigned int CreateShader(const std::string &vertexShader,
                                 const std::string &fragmentShader) {
  unsigned int program =
      glCreateProgram(); // Criar um shader program e retorna um id
  unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShader);
  unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);

  // Vamos colocar nossos dois shaders no shader program
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  // Vamos agora fazer o link do shader program
  glLinkProgram(program);
  glValidateProgram(program);

  // Agora que já fizemos o link do programa, os shaders estão dentro dele e
  // já não precisamos mais deles
  glDeleteShader(vs);
  glDeleteShader(fs);

  return program;
}
void ListMesh::InitBuffers() {
    // PROTEÇÃO: Garante que temos vértices e índices
    if (vertices.empty() || indices.empty()) return;

    // 1. Pega os vértices únicos (Apenas Posições x, y, z)
    auto flatData = FlattenVertices();

    // 2. Garante compatibilidade de tipo de dado para o EBO!
    // Se o seu mesh->indices for std::vector<int>, nós criamos um buffer unsigned int
    // Isso evita o bug da "teia de aranha" na leitura de bits do OpenGL.
    std::vector<unsigned int> glIndices(indices.begin(), indices.end());

    // 3. Cria e vincula o VAO (Ele vai "gravar" as configurações do VBO e EBO)
    glGenVertexArrays(1, &this->VAO);
    glBindVertexArray(this->VAO);

    // 4. VBO (Vertex Buffer Object) - Envia os vértices flatData
    glGenBuffers(1, &this->VBO);
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, flatData.size() * sizeof(float), flatData.data(), GL_STATIC_DRAW);

    // 5. EBO (Element Buffer Object) - Envia a ordem de ligação (índices)
    glGenBuffers(1, &this->EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO); // IMPORTANTE: Deve ser feito com o VAO ativo!
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, glIndices.size() * sizeof(unsigned int), glIndices.data(), GL_STATIC_DRAW);

    // 6. Avisa o shader onde está a Posição (Location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // ==========================================
    // SHADERS
    // ==========================================
    this->shader = std::make_unique<ShaderRC>("../shaders/default.vs", "../shaders/default.fs");
    
    //unbind VAO
    glBindVertexArray(0); 

    std::cout << "EBO Iniciado! Vértices: " << vertices.size() << " | Índices: " << indices.size() << std::endl;
}
void ListMesh::UpdateBuffers() {
    if(this->VBO == 0) return; // Evita atualizar buffer que não existe
    auto flatData = FlattenVertices();
    
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, flatData.size() * sizeof(float), flatData.data(), GL_STATIC_DRAW);
}
void ListMesh::Draw(glm::mat4 view, float fov) {
    if (this->VAO == 0 || this->indices.empty()) return;
    glEnable(GL_DEPTH_TEST);
    
    this->shader->use();
   
    
    glm::mat4 model = glm::mat4(1.0f);
    
    glm::mat4 projection = glm::perspective(glm::radians(fov), (float)800 / (float)600, 0.1f, 1000.0f);
    glm::mat4 mvp = projection * view * model;

    this->shader->setMat4("u_MVP",mvp);

   
    glBindVertexArray(this->VAO);
    
    
    glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, (void*)0);
    
    glBindVertexArray(0);
}