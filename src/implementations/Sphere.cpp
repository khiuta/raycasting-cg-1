#include "../../utils/Sphere.hpp"
#include "../../utils/Point3.hpp"
#include "../../utils/Vector3.hpp"
#include <cmath>

Sphere::Sphere(){
  this->center = Point4(0, 0, 0);
  this->radius = 0;
}

Sphere::Sphere(const Point4 &center, float radius, const Point3 &color, const Point3 &diffuse_color, const Point3 &specular_color)
{
  this->center = center;
  this->radius = radius;
  this->color = color;
  this->diffuse_color = diffuse_color;
  this->specular_color = specular_color;
}

bool Sphere::Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const{
  Vector4 w = origin - center;
  float b = 2.0f * dot(dir, w);
  float c = dot(w, w) - radius * radius;

  float disc = b * b - 4.0f * c;
  if(disc < 0){
    return false;
  }

  float sqrtD = std::sqrt(disc);
  float t1 = (-b + sqrtD) * 0.5f;
  float t2 = (-b - sqrtD) * 0.5f;

  if(t2 > t_min && t2 < t_max){
    hr.t = t2;
    hr.p_int = Point4(origin + t2*dir);
    Vector4 point_normal = hr.p_int - center;
    hr.normal = normalize(point_normal);
    hr.obj_ptr = this;
  }
  else if(t1 > t_min && t1 < t_max) {
    hr.t = t1;
    hr.p_int = Point4(origin + t1*dir);
    Vector4 point_normal = hr.p_int - center;
    hr.normal = normalize(point_normal);
    hr.obj_ptr = this;
  } else return false;

  return true;
}