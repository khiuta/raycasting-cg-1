#include "../../utils/ListMesh.hpp"
#include "../../utils/BVH.hpp"
#include <iostream>

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

    
    this->centroid = Point4((minX + maxX) / 2.0f, (minY + maxY) / 2.0f, (minZ + maxZ) / 2.0f, 1.0f);

    // 4. RECONSTRUÇÃO DA HIERARQUIA (BVH)
    this->aabb.buildBVH(10); 
}

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

void ListMesh::InitBuffers() {
    auto flatData = FlattenVertices(); 

    // PROTEÇÃO: Vamos garantir que os índices vieram do OBJ!
    if(flatData.empty() || this->indices.empty()) {
        std::cout << "ERRO: Tentou iniciar buffers com malha vazia ou sem indices!" << std::endl;
        return;
    }

    // Criação das Cores: Vamos criar um array do mesmo tamanho dos vértices
    // e forçar todo mundo a ser Vermelho (R=255, G=0, B=0, A=255)
    std::vector<unsigned char> colors;
    colors.reserve(vertices.size() * 4);
    for(size_t i = 0; i < vertices.size(); i++) {
        colors.push_back(255); // Red
        colors.push_back(0);   // Green
        colors.push_back(0);   // Blue
        colors.push_back(255); // Alpha
    }

    this->VAO = rlLoadVertexArray();
    rlEnableVertexArray(VAO);
    
    // 1. VBO de Posições (Location 0)
    this->VBO = rlLoadVertexBuffer(flatData.data(), flatData.size() * sizeof(float), true);
    rlSetVertexAttribute(0, 3, RL_FLOAT, 0, 0, 0); 
    rlEnableVertexAttribute(0);

    // 2. VBO de Cores (Location 3) - ISSO VAI EVITAR A CAMUFLAGEM
    unsigned int colorVBO = rlLoadVertexBuffer(colors.data(), colors.size() * sizeof(unsigned char), false);
    // Note o '1' no 4º parametro: Ele normaliza a cor de 0-255 para 0.0-1.0 no shader!
    rlSetVertexAttribute(3, 4, RL_UNSIGNED_BYTE, 1, 0, 0); 
    rlEnableVertexAttribute(3);
    
    // 3. EBO de Índices
    this->EBO = rlLoadVertexBufferElement(this->indices.data(), this->indices.size() * sizeof(unsigned short), true);
    
    rlDisableVertexArray();
    
    
    std::cout << "Buffers do OpenGL criados com sucesso! Vertices: " << vertices.size() << " | Indices: " << indices.size() << std::endl;
}
void ListMesh::UpdateBuffers() {
    if(this->VBO == 0) return; // Evita atualizar buffer que não existe
    auto flatData = FlattenVertices();
    
    // Envia os novos dados para o VBO existente na GPU
    rlUpdateVertexBuffer(this->VBO, flatData.data(), flatData.size() * sizeof(float), 0);
}

void ListMesh::Draw() {
    if (this->VAO == 0 || this->indices.empty()) return;

    rlDrawRenderBatchActive();

    rlPushMatrix();

        rlDisableBackfaceCulling();

     
        rlEnableVertexArray(this->VAO);
       
        rlDrawVertexArrayElements(0, this->indices.size(), 0);
        
        rlDisableVertexArray();

       
        rlEnableBackfaceCulling();

    rlPopMatrix();
}