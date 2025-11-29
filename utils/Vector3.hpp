#ifndef VECTOR3
#define VECTOR3
#include <cmath>

class Point3;

class Vector3 {
public:
  float x, y, z = 0;
  Vector3(float x, float y, float z) : x(x), y(y), z(z) {};
  Vector3() : x(0), y(0), z(0) {};
  inline void normalize() {
    float length = std::sqrt(x * x + y * y + z * z);
    if (length != 0.0f) {
        x /= length;
        y /= length;
        z /= length;
    }
  }
  inline float length() { return std::sqrt(x * x + y * y + z * z); };
  inline const float& operator[](int i) const {
    if(i == 0) return this->x;
    if(i == 1) return this->y;
    return this->z;
  }
};

// operadores
inline Vector3 operator*(float e, const Vector3 &o) {
  return { o.x*e, o.y*e, o.z*e };
};
inline Vector3 operator*(const Vector3 &o, float e) {
  return { o.x*e, o.y*e, o.z*e };
};

inline Vector3 operator+(float e, const Vector3 &o) {
  return { e+o.x, e+o.y, e+o.z };
};
inline Vector3 operator+(const Vector3 &o, float e) {
  return { o.x+e, o.y+e, o.z+e };
};
inline Vector3 operator+(const Vector3& o, const Vector3 &a) {
  return { o.x+a.x, o.y+a.y, o.z+a.z };
};

inline Vector3 operator-(float e, const Vector3 &o) {
  return { e-o.x, e-o.y, e-o.z };
};
inline Vector3 operator-(const Vector3 &o, float e) {
  return { o.x-e, o.y-e, o.z-e };
};
inline Vector3 operator-(const Vector3 &o, const Vector3 &a) {
  return { o.x-a.x, o.y-a.y, o.z-a.z };
};

inline float dot(const Vector3 &o, const Vector3 &a) { 
  return o.x*a.x + o.y*a.y + o.z*a.z; 
};

// funções livres

inline Vector3 reflect(const Vector3 &normal, const Vector3 &luz) {
  return 2*dot(normal, luz)*normal-luz;
}

inline Vector3 normalize(const Vector3 &o){
  float length = std::sqrt(o.x * o.x + o.y * o.y + o.z * o.z);
  if(length != 0.0f){
      Vector3 unit_vector(o.x/length, o.y/length, o.z/length);
      return unit_vector;
  }
  return o;
}
inline Vector3 cross(const Vector3 &s1, const Vector3 &s2){
  double x_cross = s1.y*s2.z-s1.z*s2.y;
  double y_cross = s1.z*s2.x-s1.x*s2.z;
  double z_cross = s1.x*s2.y-s1.y*s2.x;
  Vector3 cross_vector(x_cross, y_cross, z_cross);

  return cross_vector;
}

#endif
