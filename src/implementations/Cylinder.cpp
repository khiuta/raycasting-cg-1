#include "../../utils/Cylinder.hpp"
#include <cmath>
#include <algorithm>

Cylinder::Cylinder(Point4 c_base, float h, float r, Vector4 dc, bool b_lid, bool u_lid, 
                   Point3 col, Point3 dif, Point3 spec) 
    : baseCenter(c_base), height(h), radius(r), bottom_lid(b_lid), upper_lid(u_lid),
      color(col), diffuse_color(dif), specular_color(spec) {
    this->direction = dc;
    this->direction.normalize();
}

bool Cylinder::Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const {
    Vector4 w = origin - baseCenter;

    Vector4 dir_proj = dir - direction * dot(dir, direction);
    float a = dot(dir_proj, dir_proj);

    Vector4 w_proj = w - direction * dot(w, direction);
    float b = 2.0f * dot(dir_proj, w_proj);

    float c = dot(w_proj, w_proj) - radius * radius;

    float disc = b * b - 4.0f * a * c;
    if (disc < 0) return false;

    float sqrt_disc = std::sqrt(disc);
    float t1 = (-b - sqrt_disc) / (2.0f * a);
    float t2 = (-b + sqrt_disc) / (2.0f * a);

    float t_hit = -1.0f;
    Vector4 final_normal;

    for (float t : {t1, t2}) {
        if (t < t_min || t > t_max) continue;

        Point4 p_int = origin + dir * t;
        Vector4 v_int = p_int - baseCenter;
        float projection = dot(v_int, direction);

        if (projection >= 0 && projection <= height) {
            t_hit = t;
            final_normal = p_int - (baseCenter + direction * projection);
            final_normal.normalize();
            break;
        }
    }

    if (bottom_lid) {
        float t_b = dot(baseCenter - origin, direction) / dot(dir, direction);
        if (t_b > t_min && t_b < t_max && (t_hit < 0 || t_b < t_hit)) {
            Point4 p_b = origin + dir * t_b;
            if ((p_b - baseCenter).length() <= radius) {
                t_hit = t_b;
                final_normal = -direction;
            }
        }
    }

    if (upper_lid) {
        Point4 topCenter = baseCenter + direction * height;
        float t_u = dot(topCenter - origin, direction) / dot(dir, direction);
        if (t_u > t_min && t_u < t_max && (t_hit < 0 || t_u < t_hit)) {
            Point4 p_u = origin + dir * t_u;
            if ((p_u - topCenter).length() <= radius) {
                t_hit = t_u;
                final_normal = direction;
            }
        }
    }

    if (t_hit > 0) {
        hr.t = t_hit;
        hr.p_int = origin + dir * t_hit;
        hr.normal = final_normal;
        hr.obj_ptr = this;
        
        if (dot(hr.normal, dir) > 0) hr.normal = -hr.normal;
        return true;
    }

    return false;
}