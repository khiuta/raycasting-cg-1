#ifndef CONE_HPP
#define CONE_HPP

#include "Object.hpp"
#include "Vector4.hpp"

class Cone : public Object {
public:
    Point4 center; 
    Point4 vertice;
    float radius;
    float height;
    Vector4 dc;
    bool has_bottom;

    Point3 color;
    Point3 diffuse_color;
    Point3 specular_color;

    Cone(Point4 c_base, float r, bool bottom, Point4 v, Point3 col, Point3 dif, Point3 spec);

    const Point3& getColor() const override { return color; }
    const Point3& getDiffuse() const override { return diffuse_color; }
    const Point3& getSpecular() const override { return specular_color; }

    bool Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const override;
};

#endif