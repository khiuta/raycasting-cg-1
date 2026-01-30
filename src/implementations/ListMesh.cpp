#include "../../utils/ListMesh.hpp"
#include "../../utils/BVH.hpp"
#include <iostream>

ListMesh::ListMesh() {};

ListMesh::ListMesh(const std::string &filename) {
  texture->filename = filename;
  texture->loadTexture();
};

ListMesh::ListMesh(std::vector<std::unique_ptr<Triangle>> faces, std::vector<std::unique_ptr<Point4>> vertices, Point4 centroid, AABB aabb)
  : faces(std::move(faces)), vertices(std::move(vertices)), centroid(centroid), aabb(std::move(aabb)) {};

bool ListMesh::Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const {
  return aabb.Hit(origin, dir, t_min, t_max, hr);
}

void ListMesh::rebuildStructures() {
    if (faces.empty()) return;

    // 1. REPOVOAR A LISTA DE TRIÂNGULOS DA AABB RAIZ
    // O buildBVH anterior moveu os ponteiros da lista 't' da raiz para os filhos.
    // Precisamos trazer todos de volta para a raiz para reconstruir a árvore do zero.
    this->aabb.t.clear();
    for(auto& face : faces) {
        this->aabb.t.push_back(face.get());
    }

    // 2. RECÁLCULO DA AABB GLOBAL DA MALHA
    // Inicializamos com valores extremos para garantir que a caixa "encolha" até o objeto
    float minX = 1e30f, minY = 1e30f, minZ = 1e30f;
    float maxX = -1e30f, maxY = -1e30f, maxZ = -1e30f;

    for (auto& t : faces) {
        // Coletamos as coordenadas de todos os vértices do triângulo atual
        float xs[] = {t->p1.x, t->p2.x, t->p3.x};
        float ys[] = {t->p1.y, t->p2.y, t->p3.y};
        float zs[] = {t->p1.z, t->p2.z, t->p3.z};

        for(int i = 0; i < 3; i++) {
            if(xs[i] < minX) minX = xs[i];
            if(xs[i] > maxX) maxX = xs[i];
            if(ys[i] < minY) minY = ys[i];
            if(ys[i] > maxY) maxY = ys[i];
            if(zs[i] < minZ) minZ = zs[i];
            if(zs[i] > maxZ) maxZ = zs[i];
        }
    }

    // Atualizamos os limites da caixa delimitadora principal da malha
    this->aabb.min_x = minX; this->aabb.max_x = maxX;
    this->aabb.min_y = minY; this->aabb.max_y = maxY;
    this->aabb.min_z = minZ; this->aabb.max_z = maxZ;

    // 3. ATUALIZAÇÃO DO CENTRÓIDE
    // Importante para transformações que usam o centro do objeto como referência
    this->centroid = Point4((minX + maxX) / 2.0f, (minY + maxY) / 2.0f, (minZ + maxZ) / 2.0f, 1.0f);

    // 4. RECONSTRUÇÃO DA HIERARQUIA (BVH)
    // Isso vai criar os novos nós 'left' e 'right' baseados nas novas posições.
    // Note: O buildBVH geralmente limpa a lista 't' da raiz ao distribuir para os filhos.
    this->aabb.buildBVH(10); 
}

void ListMesh::applyTranslate(const Matrix4 &m) {
    bool first = true;
    for(auto& t : faces) t->applyTranslate(m);
    rebuildStructures();
}

void ListMesh::applyScale(const Matrix4 &m) {
    bool first = true;
    for(auto& t : faces) t->applyScale(m);
    rebuildStructures();
}

void ListMesh::applyRotation(const Matrix4 &m) {
  for(auto& t : faces) t->applyRotation(m);
  rebuildStructures();
}

void ListMesh::applyShear(const Matrix4 &m) {
  for(auto& t : faces) {
    t->applyShear(m);
  }
  rebuildStructures();
}
