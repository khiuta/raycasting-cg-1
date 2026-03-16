#pragma once // modern alternative to #ifndef guards
#include "../utils/Point3.hpp"
#include "../utils/Point4.hpp"
#include "../utils/Vector4.hpp"
#include "../utils/Texture.hpp"
#include "../utils/HitRecord.hpp"
#include "../utils/Triangle.hpp"


enum class Projection{
  Perspective,
  Ortographic,
  Oblique
};

enum class LightType { DIRECTIONAL, SPOTLIGHT, POINTLIGHT };

struct Light {
  LightType type;
  Point4 position;
  Vector4 direction;
  Point3 color;
  float cutoff;
  float outer_cutoff;
};

float hash(Vector4 v);
Point3 getStarryBackground(const Vector4& dir);
Point3 sampleTextureBilinear(const Texture* tex, float u, float v);
Vector4 reflect_ray(const Vector4& v, const Vector4& n);

void convertDisplayToWindow(int display_x, int display_y, float &ndc_x, float& ndc_y, float xmin, float xmax, float ymin, float ymax, int nCol, int nLin);

Point3 setColor(const Vector4 &d, HitRecord rec, std::vector<Light> lights, Point3 amb_light, std::vector<std::unique_ptr<Object>> &world);

Point3 cast_ray(const Point4& ray_origin, const Vector4& ray_dir, int depth, std::vector<std::unique_ptr<Object>> &world, std::vector<Light> lights, Point3 amb_light);

void raycast(std::ofstream &image, int lin_start, int col_start, int width, int height, float xmin, float xmax, float ymin, float ymax, int nCol, int nLin, 
              Projection projectionType, Point4 lookFrom, Vector4 u, Vector4 v_cam, Vector4 w, float dWindow, std::vector<std::unique_ptr<Object>> &world, 
              std::vector<Light> lights, Point3 amb_light);

void fill_xyz(std::string line, float &x, float &y, float &z);
void read_obj_file(const std::string& filename,
                   std::vector<std::unique_ptr<Point4>> &v,
                   std::vector<std::unique_ptr<Vector4>> &vn,
                   std::vector<std::unique_ptr<Point3>> &vt,
                   std::vector<std::unique_ptr<Triangle>> &f,
                   Point4 &centroid,
                   AABB &aabb,
                   ListMesh *mesh);