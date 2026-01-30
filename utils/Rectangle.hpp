#ifndef RECTANGLE_HPP
#define RECTANGLE_HPP

#include "Object.hpp"
#include "Vector4.hpp"

class Rectangle : public Object {
public:
    Point4 p0;
    Vector4 edge_a;
    Vector4 edge_b;
    Vector4 normal;
    Point3 color, diffuse, specular;
    float reflectivity;

    Rectangle(const Point4& p0, const Point4& p1, const Point4& p2, 
              const Point3& color, const Point3& diffuse, const Point3& specular, 
              float reflectivity = 0.0f);

    const Point3& getColor() const override { return color; }
    const Point3& getDiffuse() const override { return diffuse; }
    const Point3& getSpecular() const override { return specular; }
    float getReflectivity() const override { return reflectivity; }

    bool Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const override;
};

#endif