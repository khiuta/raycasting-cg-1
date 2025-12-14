#include "../../utils/AABB.hpp"
#include "../../utils/BVH.hpp"
#include <cmath>

const bool AABB::IntersectRayAABB(const Point4& origin, const Vector4& dir, float& t_hit) const{
  float t1 = (min_x - origin.x) / dir.x;
  float t2 = (max_x - origin.x) / dir.x;
  float t3 = (min_y - origin.y) / dir.y;
  float t4 = (max_y - origin.y) / dir.y;
  float t5 = (min_z - origin.z) / dir.z;
  float t6 = (max_x - origin.z) / dir.z;

  float t_min = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
  float t_max = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

  if (t_max < 0) return false;
  if (t_min > t_max) return false;
  t_hit = t_min;
  return true; 
}

int point_inside_aabb(Point4 &p, AABB aabb){
  if(p.x >= aabb.min_x && p.x <= aabb.max_x){
    if(p.y >= aabb.min_y && p.y <= aabb.max_y){
      if(p.z >= aabb.min_z && p.z <= aabb.max_z) return 1;
    }
  } else return 0;
}

bool triangle_inside_aabb(Triangle &tri, AABB aabb){
  int p1_inside = point_inside_aabb(tri.p1, aabb);
  int p2_inside = point_inside_aabb(tri.p2, aabb);
  int p3_inside = point_inside_aabb(tri.p3, aabb);
  if(p1_inside + p2_inside + p3_inside >= 2) return true;
  else return false;
}

const BVH AABB::buildBVH() const{
  BVH bvh;
  bvh.t = this->t;

  float h = this->height();
  float w = this->width();
  float d = this->depth();

  if(h > w && h > d) {
    AABB laabb(min_x, max_x, min_y, (max_y + min_y)/2, min_z, max_z);
    AABB uaabb(min_x, max_x, (max_y+min_y)/2, max_y, min_z, max_z);
    bvh.node1 = laabb;
    bvh.node2 = uaabb;
  } else if(w > h && w > d){
    AABB laabb(min_x, (max_x+min_x)/2, min_y, max_y, min_z, max_z);
    AABB raabb((max_x+min_x)/2, max_x, min_y, max_y, min_z, max_z);
    bvh.node1 = laabb;
    bvh.node2 = raabb;
  } else if(d > w && d > h) {
    AABB faabb(min_x, max_x, min_y, max_y, min_z, (max_z + min_z)/2);
    AABB baabb(min_x, max_x, min_y, max_y, (max_z + min_z)/2, max_z);
    bvh.node1 = faabb;
    bvh.node2 = baabb;
  }

  for(const auto& tri : this->t){
    if(triangle_inside_aabb(*tri, bvh.node1)) bvh.node1.t.push_back(tri);
    else bvh.node2.t.push_back(tri);
  }

  return bvh;
}