#include "../../utils/ListMesh.hpp"
#include <iostream>

ListMesh::ListMesh(std::vector<std::unique_ptr<Triangle>> faces, std::vector<std::unique_ptr<Point4>> vertices, Point4 centroid, Sphere SBB) 
  : faces(std::move(faces)), vertices(std::move(vertices)), centroid(centroid), SBB(SBB) {};

bool ListMesh::Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const {
  HitRecord temp;
  if(!SBB.Intersect(origin, dir, t_min, t_max, temp)) return false;

  float closest_so_far = t_max;
  bool hit_anything = false;
  for(const auto& face : this->faces) {
    HitRecord temp_rec;
    if(face->Intersect(origin, dir, t_min, closest_so_far, temp_rec)){
      hit_anything = true;
      closest_so_far = temp_rec.t;
      hr = temp_rec;
    }
  }

  if(hit_anything) return true;
  
  return false;
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
  SBB.radius *= 45;
}

void ListMesh::applyRotation(const Matrix4 &m) {
  for(auto& t : faces){
    t->applyRotation(m);
  }
}