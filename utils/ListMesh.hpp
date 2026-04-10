#ifndef LISTMESH
#define LISTMESH
#include <vector>
#include <memory>
#include "Object.hpp"
#include "Matrix4.hpp"
#include "AABB.hpp"
#include "Texture.hpp"
#include <rlgl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
class Triangle;

class ListMesh : public Object {
  public:
    std::vector<std::unique_ptr<Triangle>> faces;
    std::vector<unsigned short> indices;
    std::vector<std::unique_ptr<Point4>> vertices;
    unsigned int VAO,VBO,EBO,shader;
    Point4 centroid;
    AABB aabb;
    Texture *texture = new Texture();

    ListMesh();
    ListMesh(const std::string &filename);
    ListMesh(std::vector<std::unique_ptr<Triangle>> faces, std::vector<std::unique_ptr<Point4>> vertices, Point4 centroid, AABB aabb);

    bool Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const override;

    void rebuildStructures();
    void applyTranslate(const Matrix4 &m);
    void applyScale(const Matrix4 &m);
    void applyRotation(const Matrix4 &m);
    void applyShear(const Matrix4 &m);

    std::vector<float> FlattenVertices();
    void InitBuffers();
    void UpdateBuffers();
    void Draw(glm::mat4 view);
    const Point3& getColor() const override { return Point3(0, 0, 0); };
    const Point3& getDiffuse() const override { return Point3(0, 0, 0); };
    const Point3& getSpecular() const override { return Point3(0, 0, 0); };
};

#endif
