#ifndef TRIANGLE
#define TRIANGLE
#include "Object.hpp"
#include "Point3.hpp"
#include "Matrix4.hpp"
#include <random>
#include <chrono>

class Triangle : public Object {
  public:
    Point4 p1, p2, p3;
    Point3 color, dif_color, spec_color;
    Vector4 e1, e2, e3, normal;
    double area;

    Triangle(const Point4 &p1, const Point4 &p2, const Point4 &p3, const Vector4 &n);

    bool Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const override;

    void applyTranslate(const Matrix4 &m);
    void applyScale(const Matrix4 &m);
    void applyRotation(const Matrix4 &m);

    const Point3 getColor() const override { return color; };
    const Point3 getDiffuse() const override { return dif_color; };
    const Point3 getSpecular() const override { return spec_color; };
};

#endif