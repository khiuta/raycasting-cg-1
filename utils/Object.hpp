#ifndef OBJECT
#define OBJECT
#include "Vector4.hpp"
#include "Point4.hpp"
#include "Point3.hpp"
#include "HitRecord.hpp"

class Object {
  public:
    virtual ~Object() = default;
    virtual bool Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const = 0;
    virtual const Point3 getColor() const = 0;
    virtual const Point3 getDiffuse() const = 0;
    virtual const Point3 getSpecular() const = 0;
};

#endif