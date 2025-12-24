#ifndef AABB_HPP
#define AABB_HPP
#include "Point4.hpp"
#include "Vector4.hpp"
#include "HitRecord.hpp"
#include <vector>
#include <memory>

struct BVH;
class Triangle;

class AABB {
  public:
    float min_x, max_x, min_y, max_y, min_z, max_z;
    // t is the vector with all the triangles of the mesh inside this aabb
    std::unique_ptr<AABB> left = nullptr;
    std::unique_ptr<AABB> right = nullptr;
    std::vector<Triangle*> t;

    AABB(float min_x, float max_x, float min_y, float max_y, float min_z, float max_z)
    : min_x(min_x), max_x(max_x), min_y(min_y), max_y(max_y), min_z(min_z), max_z(max_z) {};
    AABB() : min_x(0), max_x(0), min_y(0), max_y(0), min_z(0), max_z(0) {};

    bool IntersectRayAABB(const Point4& origin, const Vector4& dir, float& t_hit) const;
    void buildBVH(int depth);
    void refit();
    bool is_leaf() const;
    bool Hit(const Point4& origin, const Vector4& dir, float t_min, float t_max, HitRecord& rec) const;

    const float height() const { return max_y - min_y; };
    const float width() const { return max_x - min_x; };
    const float depth() const { return max_z - min_z; };
};

#endif