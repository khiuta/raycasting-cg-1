#ifndef SPHERE
#define SPHERE
#include "Object.hpp"

class Sphere : public Object {
    public:
        Point4 center;
        Point3 color, diffuse_color, specular_color;
        float radius;

        Sphere();
        Sphere(const Point4 &center, float radius, const Point3 &color, const Point3 &diffuse_color, const Point3 &specular_color);

        bool Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const override;

        const Point3& getColor() const override { return color; };
        const Point3& getDiffuse() const override { return diffuse_color; };
        const Point3& getSpecular() const override { return specular_color; };
};

#endif