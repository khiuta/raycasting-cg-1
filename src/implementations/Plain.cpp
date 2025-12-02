#include "../../utils/Plain.hpp"
#include <cmath>

Plain::Plain(const Point4& p, const Vector4& normal, const Point3& color, const Point3 &diffuse_color, const Point3 &specular_color) 
: p0(p), p1(p), p2(p), normal(normal), color(color), diffuse_color(diffuse_color), specular_color(specular_color) {};

Plain::Plain(const Point4 &p0, const Point4 &p1, const Point4 &p2, const Point3 &color, const Point3 &diffuse_color, const Point3 &specular_color){
    this->p0 = p0;
    this->p1 = p1;
    this->p2 = p2;
    this->color = color;
    this->diffuse_color = diffuse_color;
    this->specular_color = specular_color;

    Vector4 s1 = p1 - p0;
    Vector4 s2 = p2 - p0;
    Vector4 plain_normal = cross(s1, s2);
    plain_normal.normalize();
    this->normal = plain_normal;
}

bool Plain::Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const{
  if(dot(normal, dir) >= 0) return false;
    float denominator = dot(this->normal, dir);

    if (std::abs(denominator) > 0.0001f) {
        Vector4 p0_to_origin = this->p0 - origin;
        float t = dot(p0_to_origin, this->normal) / denominator;
        
        if(t > t_min && t < t_max){
            hr.t = t;
            hr.p_int = Point4(origin + t*dir);
            hr.normal = this->normal;
            hr.obj_ptr = this;
        } else return false;

        return true;
    }

    return false;
}