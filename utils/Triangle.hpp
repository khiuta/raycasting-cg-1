#ifndef TRIANGLE
#define TRIANGLE
#include "Object.hpp"
#include "Point3.hpp"
#include "Matrix4.hpp"
#include <random>
#include <chrono>
#include <string>
#include "ListMesh.hpp"

class Triangle : public Object {
  public:
    Point4 p1, p2, p3;
    Point3 color, dif_color, spec_color, vt1, vt2, vt3;
    Vector4 e1, e2, e3, normal;
    double area;
    ListMesh *mesh;

    Triangle(const Point4 &p1, const Point4 &p2, const Point4 &p3, const Vector4 &n);
    Triangle(const Point4 &p1, const Point4 &p2, const Point4 &p3, const Vector4 &n, const Point3 &vt1, const Point3 &vt2, const Point3 &vt3);

    bool Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const override;

    void applyTranslate(const Matrix4 &m);
    void applyScale(const Matrix4 &m);
    void applyRotation(const Matrix4 &m);

    const Vector4 getNormal() const { return normal; };

    const Point3& getColor() const override { return color; };
    const Point3& getDiffuse() const override { return dif_color; };
    const Point3& getSpecular() const override { return spec_color; };

    void SetMesh(ListMesh *mesh) { this->mesh = mesh; };
};

#endif