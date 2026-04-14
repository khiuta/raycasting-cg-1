#include "../../utils/Triangle.hpp"
#include <iostream>
#include <cmath>

Triangle::Triangle(Point4 *p1, Point4 *p2, Point4 *p3, const Vector4 &n){
  this->p1 = p1;
  this->p2 = p2;
  this->p3 = p3;
  this->normal = n;
  this->normal.normalize();
  
  Vector4 r1 = *p2 - *p1;
  Vector4 r2 = *p3 - *p1;
  this->area = cross(r1, r2).length() / 2.0;
  
  Vector4 e2_vec = *p3 - *p2;
  Vector4 e3_vec = *p3 - *p1;
  
  this->e1 = r1;
  this->e2 = e2_vec;
  this->e3 = e3_vec;
  
  float r = 0.4f;
  float g = 0.4f;
  float b = 0.4f;
  this->color = Point3(r, g, b);
  this->dif_color = Point3(r, g, b);
  this->spec_color = Point3(.7, .7, .7);
}

Triangle::Triangle(Point4 *p1, Point4 *p2, Point4 *p3, const Vector4 &n, const Point3 &vt1, const Point3 &vt2, const Point3 &vt3){
  this->p1 = p1;
  this->p2 = p2;
  this->p3 = p3;
  this->vt1 = vt1;
  this->vt2 = vt2;
  this->vt3 = vt3;
  this->normal = n;
  this->normal.normalize();
  
  Vector4 r1 = *p2 - *p1;
  Vector4 r2 = *p3 - *p1;
  this->area = cross(r1, r2).length() / 2.0;
  
  Vector4 e2_vec = *p3 - *p2;
  Vector4 e3_vec = *p3 - *p1;
  
  this->e1 = r1;
  this->e2 = e2_vec;
  this->e3 = e3_vec;
  
  float r = 0.4f;
  float g = 0.4f;
  float b = 0.4f;
  this->color = Point3(r, g, b);
  this->dif_color = Point3(r, g, b);
  this->spec_color = Point3(.7, .7, .7);
}

bool Triangle::Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const {
  if(dot(normal, dir) >= 0) return false;
  
  float denominator = dot(this->normal, dir);
  double t = 0;

  if(std::abs(denominator) > 0.0001f){
    Vector4 p1_to_origin = *(this->p1) - origin;
    t = dot(p1_to_origin, this->normal) / denominator; 
  } else return false;

  if (t <= t_min || t >= t_max) return false;

  Point4 p_int = origin + t*dir;

  Vector4 s1 = p_int - *(this->p1);
  Vector4 s2 = p_int - *(this->p2);
  Vector4 s3 = p_int - *(this->p3);

  double c1 = dot(this->normal, cross(s3, s1)) / (2 * this->area);
  double c2 = dot(this->normal, cross(s1, s2)) / (2 * this->area);
  double c3 = 1.0 - c2 - c1;

  if(c1 < 0 || c2 < 0 || c3 < 0){
    return false;
  } 

  Point3 uv;
  uv.x = c3*vt1.x + c1*vt2.x + c2*vt3.x;
  uv.y = c3*vt1.y + c1*vt2.y + c2*vt3.y;

  if (this->mesh != nullptr && this->mesh->texture != nullptr && !this->mesh->texture->colors.empty()) {
      Texture* tex = this->mesh->texture;
      
      float u_val = uv.x - std::floor(uv.x);
      float v_val = 1.0f - (uv.y - std::floor(uv.y));

      int tex_u = (int)(u_val * (tex->width - 1));
      int tex_v = (int)(v_val * (tex->height - 1));

      tex_u = std::max(0, std::min(tex_u, tex->width - 1));
      tex_v = std::max(0, std::min(tex_v, tex->height - 1));

      uint8_t alpha = std::get<3>(tex->colors[tex_v][tex_u]);

      if (alpha < 10) {
          return false; 
      }
  }

  hr.normal = this->normal;
  if(dot(hr.normal, dir) > 0){
    hr.normal = -hr.normal;
  }
  hr.t = t;
  hr.p_int = p_int;
  hr.obj_ptr = this;
  hr.uv = uv;
  
  if (this->mesh != nullptr) {
      hr.texture = this->mesh->texture;
  } else {
      hr.texture = nullptr;
  }

  return true;
}

void Triangle::recalculateProperties() {
    this->e1 = *(this->p2) - *(this->p1); 
    this->e2 = *(this->p3) - *(this->p2);
    this->e3 = *(this->p3) - *(this->p1);

    Vector4 newNormal = cross(this->e1, this->e3); 
    this->normal = newNormal;
    this->normal.normalize();

    this->area = newNormal.length() / 2.0; 
}