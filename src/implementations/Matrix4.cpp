#include "../../utils/Matrix4.hpp"
#include <iostream>

Matrix4::Matrix4() {
  cols[0] = Vector4(1, 0, 0, 0);
  cols[1] = Vector4(0, 1, 0, 0);
  cols[2] = Vector4(0, 0, 1, 0);
  cols[3] = Vector4(0, 0, 0, 1);
}

Matrix4 translate(const Vector4 &v){
  Matrix4 m;
  m.cols[3] = Vector4(v.x, v.y, v.z, 1);
  return m;
}

Matrix4 scale(const Vector4 &v) {
  Matrix4 m;
  m.cols[0].x = v.x;
  m.cols[1].y = v.y;
  m.cols[2].z = v.z;
  return m;
}

Matrix4 rotate(const Vector4 &v, float theta){
  Vector4 u = normalize(v);
  Matrix4 vec_matrix = u.vec_matrix();
  Matrix4 m;
  Matrix4 identity;
  m.cols[0] = Vector4(0, u.z, -u.y, 0);
  m.cols[1] = Vector4(-u.z, 0, u.x, 0);
  m.cols[2] = Vector4(u.y, -u.x, 0, 0);
  m.cols[3] = Vector4(0, 0, 0, 0);
  Matrix4 m_rotation = vec_matrix + (identity - vec_matrix)*std::cos(theta) + m*std::sin(theta);
  m.cols[0].w = 0;
  m.cols[1].w = 0;
  m.cols[2].w = 0;
  m.cols[3] = Vector4(0, 0, 0, 1.0f);
  
  return m_rotation;
}