#ifndef PLAIN
#define PLAIN
#include "Object.hpp"

class Plain : public Object {
    public:
        Point4 p0, p1, p2;
        Vector4 normal;
        Point3 color;
        Point3 diffuse_color;
        Point3 specular_color;

        Plain(const Point4& p, const Vector4& normal, const Point3& color, const Point3 &diffuse_color, const Point3 &specular_color);
        Plain(const Point4 &p0, const Point4 &p1, const Point4 &p2, const Point3 &color, const Point3 &diffuse_color, const Point3 &specular_color);

        const Point3& getColor() const override { return color; };
        const Point3& getDiffuse() const override { return diffuse_color; };
        const Point3& getSpecular() const override { return specular_color; };

        Vector4 getSurfaceNormal() const { return normal; };
        bool Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const override;
};

#endif