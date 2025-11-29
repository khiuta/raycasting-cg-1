#ifndef MATRIX4
#define MATRIX4
#include "Vector4.hpp"
#include <cmath>

class Matrix4 {
  public:
    Vector4 cols[4];

    Matrix4(); // identity matrix

    inline Vector4 operator*(const Vector4 &v) const{
      return (this->cols[0] * v.x) +
             (this->cols[1] * v.y) +
             (this->cols[2] * v.z) +
             (this->cols[3] * v.w);
    }
    inline Matrix4 operator*(float f) const {
      Matrix4 m_result;
      for(int i = 0; i < 4; i++){
        m_result.cols[i] = Vector4(
          this->cols[i].x * f,
          this->cols[i].y * f,
          this->cols[i].z * f,
          this->cols[i].w * f
        );
      }
      return m_result;
    }
    inline Matrix4 operator+(const Matrix4 &m) const {
      Matrix4 m_result;
      for(int i = 0; i < 4; i++){
        m_result.cols[i] = Vector4(
          this->cols[i].x + m.cols[i].x,
          this->cols[i].y + m.cols[i].y,
          this->cols[i].z + m.cols[i].z,
          this->cols[i].w + m.cols[i].w
        );
      }
      return m_result;
    }
    inline Matrix4 operator-(const Matrix4 &m){
      Matrix4 m_result;
      for(int i = 0; i < 4; i++){
        m_result.cols[i] = Vector4(
          this->cols[i].x - m.cols[i].x,
          this->cols[i].y - m.cols[i].y,
          this->cols[i].z - m.cols[i].z,
          this->cols[i].w - m.cols[i].w
        );
      }
      return m_result;
    }
};

Matrix4 translate(const Vector4 &v);
Matrix4 scale(const Vector4 &v);
Matrix4 rotate(const Vector4 &v, float theta);

#endif