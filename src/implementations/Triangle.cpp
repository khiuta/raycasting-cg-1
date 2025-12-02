#include "../../utils/Triangle.hpp"

float random_float() {
    static std::mt19937 generator(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()
    );
    
    static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    
    return distribution(generator);
}

Triangle::Triangle(const Point4 &p1, const Point4 &p2, const Point4 &p3, const Vector4 &n){
  this->p1 = p1;
  this->p2 = p2;
  this->p3 = p3;
  this->normal = n;
  this->normal.normalize();
  Vector4 r1 = p2 - p1;
  Vector4 r2 = p3 - p1;
  this->area = cross(r1, r2).length()/2;
  Vector4 e2 = p3 - p2;
  Vector4 e3 = p3 - p1;
  this->e1 = r1;
  this->e2 = e2;
  this->e3 = e3;
  float r = random_float();
  float g = random_float();
  float b = random_float();
  this->color = Point3(0.35, 0.35, 0.35);
  this->dif_color = Point3(0.35, 0.35, 0.35);
  this->spec_color = Point3(.1, .1, .1);
}

bool Triangle::Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const {
  if(dot(normal, dir) >= 0) return false;
  float denominator = dot(this->normal, dir);
  double t = 0;
  if(std::abs(denominator) > 0.0001f){
    Vector4 p1_to_origin = this->p1 - origin;
    t = dot(p1_to_origin, this->normal)/denominator; 
  } else return false;

  Point4 p_int = origin + t*dir;

  Vector4 s1 = p_int - p1;
  Vector4 s2 = p_int - p2;
  Vector4 s3 = p_int - p3;

  double c1 = dot(this->normal, cross(s3, s1))/(2*this->area);
  double c2 = dot(this->normal, cross(s1, s2))/(2*this->area);
  double c3 = 1.0 - c2 - c1;

  if(c1 < 0 || c2 < 0 || c3 < 0){
    return false;
  } else {
    if(t > t_min && t < t_max){
      hr.normal = this->normal;

      if(dot(hr.normal, dir) > 0){
        hr.normal = -hr.normal;
      }
      hr.t = t;
      hr.p_int = p_int;
      hr.obj_ptr = this;
    } else return false;

    return true;
  }
}

void Triangle::applyTranslate(const Matrix4 &m){
  Vector4 newP1 = m * Vector4(p1.x, p1.y, p1.z, 1.0f);
  Vector4 newP2 = m * Vector4(p2.x, p2.y, p2.z, 1.0f);
  Vector4 newP3 = m * Vector4(p3.x, p3.y, p3.z, 1.0f);

  p1 = Point4(newP1.x, newP1.y, newP1.z, 1.0f);
  p2 = Point4(newP2.x, newP2.y, newP2.z, 1.0f);
  p3 = Point4(newP3.x, newP3.y, newP3.z, 1.0f);

  this->e1 = p2 - p1;
  this->e2 = p3 - p2;
  this->e3 = p3 - p1;

  Vector4 newNormal = m * Vector4(normal.x, normal.y, normal.z, 0.0f);
  this->normal = normalize(newNormal);

  this->area = cross(this->e1, -this->e3).length() / 2.0;
}

void Triangle::applyScale(const Matrix4 &m){
  Vector4 newP1 = m * Vector4(p1.x, p1.y, p1.z, 1.0f);
  Vector4 newP2 = m * Vector4(p2.x, p2.y, p2.z, 1.0f);
  Vector4 newP3 = m * Vector4(p3.x, p3.y, p3.z, 1.0f);

  p1 = Point4(newP1.x, newP1.y, newP1.z, 1.0f);
  p2 = Point4(newP2.x, newP2.y, newP2.z, 1.0f);
  p3 = Point4(newP3.x, newP3.y, newP3.z, 1.0f);

  this->e1 = p2 - p1;
  this->e2 = p3 - p2;
  this->e3 = p3 - p1;

  this->normal.x /= m.cols[0].x;
  this->normal.y /= m.cols[1].y;
  this->normal.z /= m.cols[2].z;

  this->area = cross(this->e1, -this->e3).length() / 2.0;

  this->normal.normalize();
}

void Triangle::applyRotation(const Matrix4 &m){
  Vector4 newP1 = m * Vector4(p1.x, p1.y, p1.z, 1.0f);
  Vector4 newP2 = m * Vector4(p2.x, p2.y, p2.z, 1.0f);
  Vector4 newP3 = m * Vector4(p3.x, p3.y, p3.z, 1.0f);

  p1 = Point4(newP1.x, newP1.y, newP1.z, 1.0f);
  p2 = Point4(newP2.x, newP2.y, newP2.z, 1.0f);
  p3 = Point4(newP3.x, newP3.y, newP3.z, 1.0f);

  this->e1 = p2 - p1;
  this->e2 = p3 - p2;
  this->e3 = p3 - p1;

  Vector4 newNormal = m * Vector4(normal.x, normal.y, normal.z, 0.0f);
  this->normal = normalize(newNormal);

  this->area = cross(this->e1, -this->e3).length() / 2.0;
}