#ifndef LISTMESH
#define LISTMESH
#include <vector>
#include <memory>
#include "Object.hpp"
#include "Matrix4.hpp"
#include "AABB.hpp"
#include "Texture.hpp"

class Triangle;

class ListMesh : public Object {
  public:
    std::vector<std::unique_ptr<Triangle>> faces;
    std::vector<std::unique_ptr<Point4>> vertices;
    Point4 centroid;
    AABB aabb;
    Texture *texture = new Texture();

    ListMesh();
    ListMesh(const std::string &filename);
    ListMesh(std::vector<std::unique_ptr<Triangle>> faces, std::vector<std::unique_ptr<Point4>> vertices, Point4 centroid, AABB aabb);

    bool Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const override;

    void applyTranslate(const Matrix4 &m);
    void applyScale(const Matrix4 &m);
    void applyRotation(const Matrix4 &m);

    const Point3& getColor() const override { return Point3(0, 0, 0); };
    const Point3& getDiffuse() const override { return Point3(0, 0, 0); };
    const Point3& getSpecular() const override { return Point3(0, 0, 0); };
};

#endif
