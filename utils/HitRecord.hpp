#ifndef HITRECORD
#define HITRECORD
#include <string>
#include "Point3.hpp"
#include "Texture.hpp"

class Object;

struct HitRecord {
  double t;
  Point4 p_int;
  Vector4 normal;
  const Object *obj_ptr;
  Point3 uv;
  Texture *texture = nullptr;
};

#endif