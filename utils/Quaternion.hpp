#ifndef QUATERNION
#define QUATERNION
#include "Vector4.hpp"

class Quaternion {
  Vector4 v;
  float w;

  Quaternion() : v(Vector4(0, 0, 0)), w(0) {};
  Quaternion(const Vector4 &v, float w) : v(v), w(w) {};

  inline Quaternion conjugated(){
    Quaternion q(-this->v, this->w);
    return q;
  }
  inline Quaternion operator*(const Quaternion &q){
    Vector4 v = q.w*this->v + this->w*q.v + cross(this->v, q.v);
    float w = this->w*q.w - dot(this->v, q.v);
    return Quaternion(v, w);
  }
  inline Quaternion conjugated_product(){
    return Quaternion(Vector4(0, 0, 0), dot(this->v, this->v) + this->w*this->w);
  }
};

#endif