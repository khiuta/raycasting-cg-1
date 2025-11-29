#ifndef VECTOR4
#define VECTOR4
#include <cmath>
class Point4;
class Matrix4;

class Vector4 {
  public:
    float x, y, z, w;

    Vector4() : x(0), y(0), z(0), w(0) {};
    Vector4(float x, float y, float z) : x(x), y(y), z(z), w(0) {};
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {};

    inline float length() const { return std::sqrt(x*x + y*y + z*z ); };
    inline void normalize();
    inline Vector4 operator-() const {
      return Vector4(-x, -y, -z, w);
    }
    inline const float& operator[](int i) const {
      if(i == 0) return this->x;
      if(i == 1) return this->y;
      if(i == 2) return this->z;
      if(i == 3) return this->w;
      return this->x;
    }
    // vector multiplicated by its transposted, resulting in a matrix (column vec * line vec)
    Matrix4 vec_matrix() const;
};

// vector +- vector
inline Vector4 operator+(const Vector4 &v1, const Vector4 &v2){
  return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w };
}
inline Vector4 operator-(const Vector4 &v1, const Vector4 &v2){
  return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w };
}

// vector * float
inline Vector4 operator*(const Vector4 &v, float f){
  return { v.x * f, v.y * f, v.z * f, v.w * f };
}
inline Vector4 operator*(float f, const Vector4 &v){
  return { v.x * f, v.y * f, v.z * f, v.w * f };
}

// vector / float
inline Vector4 operator/(const Vector4 &v, float f){
  return { v.x / f, v.y / f, v.z / f, v.w / f };
}

inline float dot(const Vector4 &v1, const Vector4 &v2){
  return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

inline Vector4 cross(const Vector4 &v1, const Vector4 &v2){
  return Vector4(
    v1.y * v2.z - v1.z * v2.y,
    v1.z * v2.x - v1.x * v2.z,
    v1.x * v2.y - v1.y * v2.x,
    0.0f
  );
}

inline Vector4 normalize(const Vector4 &v) {
  if(v.length() > 1e-8) return v / v.length();
  return v;
}

inline void Vector4::normalize() {
  float len = this->length();
  if(this->length() > 1e-8){
    x /= len;
    y /= len;
    z /= len;
  }
}

inline Vector4 reflect(const Vector4 &normal, const Vector4 &luz){
  return 2*dot(normal, luz)*normal-luz;
}

#endif