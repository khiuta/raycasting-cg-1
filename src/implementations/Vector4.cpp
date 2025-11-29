#include "../../utils/Vector4.hpp"
#include "../../utils/Matrix4.hpp"

Matrix4 Vector4::vec_matrix() const {
  Matrix4 m;
  for(int i = 0; i < 4; i++){
    m.cols[i].x = x * operator[](i);
    m.cols[i].y = y * operator[](i);
    m.cols[i].z = z * operator[](i);
    m.cols[i].w = w * operator[](i);
  }
  return m;
}