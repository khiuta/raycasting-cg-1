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

void ListMesh::applyTranslate(const Matrix4 &m) {
    bool first = true;
    for(auto& t : faces){
        t->applyTranslate(m); // Move o triângulo

        // Atualiza os limites da AABB da ListMesh baseada no triângulo movido
        // Considerando que seu triângulo tem os pontos p1, p2, p3 acessíveis
        Point4 pts[3] = {t->p1, t->p2, t->p3};
        for (int i = 0; i < 3; i++) {
            if (first) {
                aabb.min_x = aabb.max_x = pts[i].x;
                aabb.min_y = aabb.max_y = pts[i].y;
                aabb.min_z = aabb.max_z = pts[i].z;
                first = false;
            } else {
                aabb.min_x = std::min(aabb.min_x, (float)pts[i].x);
                aabb.max_x = std::max(aabb.max_x, (float)pts[i].x);
                aabb.min_y = std::min(aabb.min_y, (float)pts[i].y);
                aabb.max_y = std::max(aabb.max_y, (float)pts[i].y);
                aabb.min_z = std::min(aabb.min_z, (float)pts[i].z);
                aabb.max_z = std::max(aabb.max_z, (float)pts[i].z);
            }
        }
    }
}

void ListMesh::applyScale(const Matrix4 &m) {
    bool first = true;
    for(auto& t : faces){
        t->applyScale(m); // Aplica escala no triângulo (vértices e normal)

        // Recalcula a AABB da malha baseada nos novos vértices dos triângulos
        Point4 pts[3] = {t->p1, t->p2, t->p3};
        for (int i = 0; i < 3; i++) {
            if (first) {
                aabb.min_x = aabb.max_x = pts[i].x;
                aabb.min_y = aabb.max_y = pts[i].y;
                aabb.min_z = aabb.max_z = pts[i].z;
                first = false;
            } else {
                aabb.min_x = std::min(aabb.min_x, (float)pts[i].x);
                aabb.max_x = std::max(aabb.max_x, (float)pts[i].x);
                aabb.min_y = std::min(aabb.min_y, (float)pts[i].y);
                aabb.max_y = std::max(aabb.max_y, (float)pts[i].y);
                aabb.min_z = std::min(aabb.min_z, (float)pts[i].z);
                aabb.max_z = std::max(aabb.max_z, (float)pts[i].z);
            }
        }
    }
    // Atualiza o centroide após a mudança de escala
    centroid.x = (aabb.max_x + aabb.min_x) / 2.0f;
    centroid.y = (aabb.max_y + aabb.min_y) / 2.0f;
    centroid.z = (aabb.max_z + aabb.min_z) / 2.0f;
}

void ListMesh::applyRotation(const Matrix4 &m) {
  for(auto& t : faces){
    t->applyRotation(m);
  }
}
