#ifndef HITRECORD
#define HITRECORD

class Object;

struct HitRecord {
  double t;
  Point4 p_int;
  Vector4 normal;
  const Object *obj_ptr;
};

#endif