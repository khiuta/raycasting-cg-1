#ifndef CILINDER_HPP
#define CILINDER_HPP

#include "Object.hpp"
#include "Vector4.hpp"

class Cylinder : public Object {
public:
    Point4 baseCenter;
    float height;
    float radius;
    Vector4 direction;
    bool bottom_lid;
    bool upper_lid;

    Point3 color;
    Point3 diffuse_color;
    Point3 specular_color;

    Cylinder(Point4 c_base, float h, float r, Vector4 dc, bool b_lid, bool u_lid, Point3 col, Point3 dif, Point3 spec);

    bool Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const override;
    
    const Point3& getColor() const override { return color; }
    const Point3& getDiffuse() const override { return diffuse_color; }
    const Point3& getSpecular() const override { return specular_color; }
};

#endif