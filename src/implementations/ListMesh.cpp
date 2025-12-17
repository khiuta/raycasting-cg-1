#include "../../utils/ListMesh.hpp"
#include "../../utils/BVH.hpp"
#include <iostream>

ListMesh::ListMesh(std::vector<std::unique_ptr<Triangle>> faces, std::vector<std::unique_ptr<Point4>> vertices, Point4 centroid, AABB aabb) 
  : faces(std::move(faces)), vertices(std::move(vertices)), centroid(centroid), aabb(std::move(aabb)) {};

bool ListMesh::Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const {
  return aabb.Hit(origin, dir, t_min, t_max, hr);
}

void ListMesh::applyTranslate(const Matrix4 &m) {
  for(auto& t : faces){
    t->applyTranslate(m);
  }
}

void ListMesh::applyScale(const Matrix4 &m) {
  for(auto& t : faces){
    t->applyScale(m);
  }
}

void ListMesh::applyRotation(const Matrix4 &m) {
  for(auto& t : faces){
    t->applyRotation(m);
  }
}