#include "../../utils/AABB.hpp"
#include <cmath>

const bool AABB::IntersectRayAABB(const Point4& origin, const Vector4& dir) const{
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
  return true; 
}