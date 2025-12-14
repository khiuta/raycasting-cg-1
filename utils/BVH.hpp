#ifndef BVH_HPP
#define BVH_HPP
#include "Triangle.hpp"
#include "AABB.hpp"
#include <vector>

struct BVH {
  std::vector<Triangle*> t;
  AABB node1;
  AABB node2;
};

#endif