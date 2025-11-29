#ifndef POINT
#define POINT
#include "Vector3.hpp"

class Point3 {
public:
  float x, y, z;
  Point3(float x, float y, float z) : x(x), y(y), z(z) {};
  Point3() : x(0), y(0), z(0) {};
  inline void clamp(){
    if(x < 0.0f) x = 0.0f;
    if(x > 1.0f) x = 1.0f;
    if(y < 0.0f) y = 0.0f;
    if(y > 1.0f) y = 1.0f;
    if(z < 0.0f) z = 0.0f;
    if(z > 1.0f) z = 1.0f;
  }
};

inline Point3 operator+(const Point3 &p, const Vector3 &o) {
  return { p.x + o.x, p.y + o.y, p.z + o.z }; 
};
inline Point3 operator+(const Vector3 &o, const Point3 &p) {
  return { o.x + p.x, o.y + p.y, o.z + p.z };
};
inline Point3 operator+(const Point3 &p1, const Point3 &p2) {
  return { p1.x + p2.x, p1.y + p2.y, p1.z + p2.z };
};
inline Point3 operator-(const Point3 &p, const Vector3 &o) {
  return { p.x - o.x, p.y - o.y, p.z - o.z }; 
};
inline Point3 operator-(const Vector3 &o, const Point3 &p) {
  return { o.x - p.x, o.y - p.y, o.z - p.z };
};
inline Vector3 operator-(const Point3 &p1, const Point3 &p2) {
  return { p1.x - p2.x, p1.y - p2.y, p1.z - p2.z };
}

inline Point3 operator*(const Point3 &p, float t) {
  return { p.x * t, p.y * t, p.z * t };
}

#endif // !POINT
