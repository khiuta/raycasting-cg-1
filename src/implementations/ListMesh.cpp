#include "../../utils/ListMesh.hpp"
#include "../../utils/BVH.hpp"
#include <iostream>

ListMesh::ListMesh(std::vector<std::unique_ptr<Triangle>> faces, std::vector<std::unique_ptr<Point4>> vertices, Point4 centroid, AABB aabb) 
  : faces(std::move(faces)), vertices(std::move(vertices)), centroid(centroid), aabb(aabb) {};

AABB ListMesh::recursive_bvh(const Point4 &origin, const Vector4 &dir, const AABB &aabb, int divs) const {
  if(divs <= 0){
    return aabb;
  }

  BVH bvh = aabb.buildBVH();
  float node1_t = 0, node2_t = 0;
  bool node1_hit = bvh.node1.IntersectRayAABB(origin, dir, node1_t);
  bool node2_hit = bvh.node2.IntersectRayAABB(origin, dir, node2_t);

  if(node1_hit && node2_hit){
    if(node1_t < node2_t) return recursive_bvh(origin, dir, bvh.node1, divs-1);
    else return recursive_bvh(origin, dir, bvh.node2, divs-1);
  } else if(node1_hit) {
    return recursive_bvh(origin, dir, bvh.node1, divs-1);
  } else if(node2_hit) {
    return recursive_bvh(origin, dir, bvh.node2, divs-1);
  }

  return aabb;
}

bool ListMesh::Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const {
  float t;
  if(!aabb.IntersectRayAABB(origin, dir, t)) return false;
  AABB bvh = recursive_bvh(origin, dir, aabb, 2);

  float closest_so_far = t_max;
  bool hit_anything = false;
  for(const auto& face : bvh.t) {
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
}

void ListMesh::applyRotation(const Matrix4 &m) {
  for(auto& t : faces){
    t->applyRotation(m);
  }
}