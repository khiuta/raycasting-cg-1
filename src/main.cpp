#include <cmath>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <numbers>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// #include "../utils/glad.h"
#define Vector3 RL_Vector3
#define Vector4 RL_Vector4
#define Matrix RL_Matrix
#define Texture RL_Texture
#define Rectangle RL_Rectangle
#define Material RL_Material

#include <raylib.h>
#include <rlgl.h> // use opengl primitive matrices

#undef Vector3
#undef Vector4
#undef Matrix
#undef Texture
#undef Rectangle
#undef Material

#include "../utils/Point4.hpp"
#include "../utils/Point3.hpp"
#include "../utils/Vector4.hpp"
#include "../utils/Triangle.hpp"
#include "../utils/Object.hpp"
#include "../utils/HitRecord.hpp"
#include "../utils/ListMesh.hpp"
#include "../utils/Plain.hpp"
#include "../utils/Rectangle.hpp"
#include "../utils/AABB.hpp"
#include "../utils/Sphere.hpp"
#include "../utils/Cylinder.hpp"
#include "../utils/Cone.hpp"

#include "./aux_functions.hpp"

const float wWindow = 4.f, hWindow = 3.f;
const int nCol = wWindow * 200, nLin = hWindow * 200;
float dx = wWindow / nCol;
float dy = hWindow / nLin;
float dWindow = 4.0f;

// auxiliary vector for real time edge detection mode
Point3 init(0.0f, 0.0f, 0.0f);
std::vector<Point3> pixels((size_t)(nCol * nLin), init);

bool edge_detection = false;

float xmin = -2.0f, xmax = 2.0f;
float ymin = -1.5f, ymax = 1.5f;

Projection projectionType = Projection::Perspective;

struct Material
{
  Point3 color;
  Point3 spec;
};

Object *selectedObject = nullptr;

std::vector<Light> lights;

Point3 amb_light(.3, .3, .3);
Point4 observer_pos(0, 0, 0);

Point4 lookFrom(45.0f, 15.0f, 85.0f);
Point4 lookAt(30.f, 5.0f, 30.0f);
// Point4 lookAt(0.f, 5.0f, 70.0f);
Vector4 vUp(0.0f, 1.0f, 0.0f, 0.0f);
Vector4 u, v_cam, w;

std::vector<std::unique_ptr<Object>> world;

std::unique_ptr<ListMesh> createMesh(const std::string &objPath, const std::string &texturePath)
{
  std::vector<std::unique_ptr<Point4>> v;
  std::vector<std::unique_ptr<Vector4>> vn;
  std::vector<std::unique_ptr<Point3>> vt;
  std::vector<std::unique_ptr<Triangle>> f;
  Point4 centroid;
  AABB aabb;
  auto mesh = std::make_unique<ListMesh>(texturePath);
  read_obj_file(objPath, v, vn, vt, f, centroid, aabb, mesh.get());
  mesh->aabb = std::move(aabb);
  mesh->faces = std::move(f);
  mesh->vertices = std::move(v);
  mesh->centroid = std::move(centroid);
  return mesh;
}

float calculate_dWindow_from_FOV(float fov_degrees, float wWindow)
{
  float fov_radians = fov_degrees * M_PI / 180.0f;
  return (wWindow / 2.0f) / std::tan(fov_radians / 2.0f);
}

float calculate_FOV_from_dWindow(float dWindow, float wWindow)
{
  float fov_radians = 2.0f * std::atan((wWindow / 2.0f) / dWindow);
  return fov_radians * 180.0f / M_PI;
}

int main()
{
  std::string obj_name = "car_1.obj";

  Point3 spec = Point3(0.5f, 0.5f, 0.5f);
  Point3 low_spec = Point3(0.1f, 0.1f, 0.1f);

  Material lamp_color;
  lamp_color.color = Point3(1.0f, 0.9f, 0.5f);
  lamp_color.spec = spec;
  Material post_color;
  post_color.color = Point3(0.2f, 0.2f, 0.2f);
  post_color.spec = spec;
  Material road_cone_color;
  road_cone_color.color = Point3(0.8f, 0.4f, 0.1f);
  road_cone_color.spec = low_spec;
  Material road_strip_color;
  road_strip_color.color = Point3(1.0f, 1.0f, 1.0f);
  road_strip_color.spec = low_spec;

  Light directional;
  directional.type = LightType::DIRECTIONAL;
  directional.direction = Vector4(-1.0f, -1.0f, -0.5f);
  directional.direction.normalize();
  directional.color = Point3(0.07f, 0.07f, 0.7f);

  Light post_spot;
  post_spot.type = LightType::SPOTLIGHT;
  post_spot.color = Point3(1.0f, 0.9f, 0.0f);
  post_spot.position = Point4(15.0f, 14.5f, 41.0f);
  post_spot.direction = Vector4(0.0f, -1.0f, 0.0f);
  post_spot.cutoff = std::cos(40.0f * M_PI / 180.0f);
  post_spot.outer_cutoff = std::cos(45.0f * M_PI / 180.0f);

  Light post_spot2;
  post_spot2.type = LightType::SPOTLIGHT;
  post_spot2.color = Point3(1.0f, 0.9f, 0.0f);
  post_spot2.position = Point4(50.0f, 14.5f, 41.0f);
  post_spot2.direction = Vector4(0.0f, -1.0f, 0.0f);
  post_spot2.cutoff = std::cos(40.0f * M_PI / 180.0f);
  post_spot2.outer_cutoff = std::cos(45.0f * M_PI / 180.0f);

  Light farol1_carro2;
  farol1_carro2.type = LightType::SPOTLIGHT;
  farol1_carro2.color = Point3(1.0f, .7f, 0.0f);
  farol1_carro2.position = Point4(28.5f, 1.5f, 34.5f);
  farol1_carro2.direction = Vector4(0.0f, -0.7f, 1.0f);
  farol1_carro2.cutoff = std::cos(8.f* M_PI / 180.0f);
  farol1_carro2.outer_cutoff = std::cos(20.f* M_PI / 180.0f);

   Light farol2_carro2;
  farol2_carro2.type = LightType::SPOTLIGHT;
  farol2_carro2.color = Point3(1.0f, 0.7f, 0.0f);
  farol2_carro2.position = Point4(30.9f, 1.5f, 34.5f);
  farol2_carro2.direction = Vector4(0.0f, -0.7f, 1.0f);
  farol2_carro2.cutoff = std::cos(8.f* M_PI / 180.0f);
  farol2_carro2.outer_cutoff = std::cos(20.f* M_PI / 180.0f);

  lights.push_back(directional);
  lights.push_back(post_spot);
  lights.push_back(post_spot2);
  lights.push_back(farol1_carro2);
  lights.push_back(farol2_carro2);


#pragma region world objects
  auto car1 = createMesh("car_1.obj", "textures/car_1.png");

  car1->applyTranslate(translate(
      Vector4(-car1->centroid.x, -car1->centroid.y, -car1->centroid.z)));
  car1->applyScale(scale(Vector4(0.1, 0.1, 0.1)));
  Vector4 A_factors(0.5f, 0.0f, 0.0f);
  Vector4 B_factors(0.0f, 0.0f, 0.0f);
  car1->applyShear(shear(A_factors, B_factors));
  car1->applyRotation(
      rotate(Vector4(0.0f, 1.0f, 0.0f), -90.0f * M_PI / 180.0f));
  float car_half_height = (car1->aabb.max_y - car1->aabb.min_y) / 2.0f;
  car1->applyTranslate(translate(Vector4(20.0f, car_half_height, 30.0f)));

  auto car2 = createMesh("fuscao.obj", "textures/gulf_blue.png");

  car2->applyTranslate(translate(
      Vector4(-car2->centroid.x, -car2->centroid.y, -car2->centroid.z)));
  car2->applyScale(scale(Vector4(2.5f, 2.5f, 2.5f)));
  car2->applyTranslate(translate(Vector4(30.0f, car_half_height, 30.0f)));

  auto car3 = createMesh("sedan.obj", "textures/sedan.png");

  car3->applyTranslate(translate(
      Vector4(-car3->centroid.x, -car3->centroid.y, -car3->centroid.z)));
  car3->applyScale(scale(Vector4(2.f, 2.f, 2.f)));
  // car3->applyRotation(
  //   rotate(Vector4(0.0f, 1.0f, 0.0f), -00.0f * M_PI / 180.0f));
  car3->applyTranslate(translate(Vector4(37.0f, car_half_height, 30.0f)));

  auto car4 = createMesh("Bus.obj", "textures/Bus.png");

  car4->applyTranslate(translate(
      Vector4(-car4->centroid.x, -car4->centroid.y, -car4->centroid.z)));
  car4->applyScale(scale(Vector4(3.f, 3.f, 3.f)));
  car4->applyRotation(
      rotate(Vector4(0.0f, 1.0f, 0.0f), -90.0f * M_PI / 180.0f));
  car4->applyTranslate(translate(Vector4(60.0f, 4.f, 45.0f)));

  auto cube = createMesh("cube.obj", "");
  cube->applyTranslate(translate(
      Vector4(-cube->centroid.x, -cube->centroid.y, -cube->centroid.z)));
  cube->applyScale(scale(Vector4(25.0f, 8.0f, 10.0f)));
  float half_cube_height = (cube->aabb.max_y - cube->aabb.min_y) / 2.0f;
  cube->applyTranslate(translate(Vector4(0.0f, half_cube_height, -25.0f)));
  for (auto &face : cube->faces)
  {
    face->reflectivity = 1.0f;
  }

  auto shop = createMesh("autocreto.obj", "textures/autocretotexture.png");
  shop->applyTranslate(translate(
      Vector4(-shop->centroid.x, -shop->centroid.y, -shop->centroid.z)));
  shop->applyScale(scale(Vector4(6.0f, 10.0f, 5.0f)));
  shop->applyRotation(rotate(Vector4(1.0f, 0.0f, 0.0f), 90.0f * M_PI / 180.0f));
  float half_shop_height = (shop->aabb.max_y - shop->aabb.min_y) / 2.0f;
  shop->applyTranslate(translate(Vector4(30.0f, half_shop_height, 5.0f)));

  auto vendinha = createMesh("vendinha.obj", "textures/vendinha.png");
  vendinha->applyTranslate(translate(
      Vector4(-vendinha->centroid.x, -vendinha->centroid.y, -vendinha->centroid.z)));
  vendinha->applyScale(scale(Vector4(4.0f, 4.0f, 4.0f)));
  vendinha->applyRotation(rotate(Vector4(0.0f, 1.0f, 0.0f), 0.0f * M_PI / 180.0f));
  vendinha->applyTranslate(translate(
      Vector4(-10.f, 4.f, 37.f)));

  auto japanesebuilding = createMesh("japanesebuilding.obj", "textures/japanesebuilding.png");
  japanesebuilding->applyTranslate(translate(
      Vector4(-japanesebuilding->centroid.x, -japanesebuilding->centroid.y, -japanesebuilding->centroid.z)));
  japanesebuilding->applyScale(scale(Vector4(3.0f, 3.0f, 3.0f)));
  japanesebuilding->applyRotation(rotate(Vector4(0.0f, 1.0f, 0.0f), 90.0f * M_PI / 180.0f));
  japanesebuilding->applyTranslate(translate(
      Vector4(-10.f, 10.f, 70.f)));

  auto road = createMesh("cube.obj", "");
  road->applyTranslate(translate(
      Vector4(-road->centroid.x, -road->centroid.y, -road->centroid.z)));
  road->applyScale(scale(Vector4(60.0f, 0.01f, 10.0f)));
  float half_road_height = (road->aabb.max_y - road->aabb.min_y) / 2.0f;
  road->applyTranslate(translate(Vector4(30.0f, half_road_height, 50.0f)));

#pragma region vegetation

  auto sidewalk = createMesh("floor.obj", "textures/floor_texture.png");
  sidewalk->applyTranslate(translate(Vector4(
      -sidewalk->centroid.x, -sidewalk->centroid.y, -sidewalk->centroid.z)));
  sidewalk->applyScale(scale(Vector4(1.0f, 1.6f, 0.5f)));
  sidewalk->applyRotation(
      rotate(Vector4(1.0f, 0.0f, 0.0f), 90.0f * M_PI / 180.0f));
  sidewalk->applyTranslate(translate(Vector4(
      10.f, 0.2f, 0.f)));

  float sidewalk_half_height =
      (sidewalk->aabb.max_y - sidewalk->aabb.min_y) / 2.0f;
  sidewalk->applyTranslate(translate(Vector4(10.0f, -0.3f, 35.0f)));
  world.push_back(std::move(sidewalk));

  auto tree = createMesh("tree01.obj", "textures/tree01_spring.png");
  tree->applyTranslate(translate(
      Vector4(-tree->centroid.x, -tree->centroid.y, -tree->centroid.z)));
  tree->applyScale(scale(Vector4(4.0f, 4.0f, 4.0f)));
  float tree_half_height = (tree->aabb.max_y - tree->aabb.min_y) / 2.0f;
  tree->applyTranslate(translate(Vector4(10.0f, 5.0f, 40.0f)));
  world.push_back(std::move(tree));

  auto bush = createMesh("bush01.obj", "textures/bush1_spring.png");
  bush->applyTranslate(translate(
      Vector4(-bush->centroid.x, -bush->centroid.y, -bush->centroid.z)));
  bush->applyScale(scale(Vector4(4.0f, 4.0f, 4.0f)));
  float bush_half_height = (bush->aabb.max_y - bush->aabb.min_y) / 2.0f;
  bush->applyTranslate(translate(Vector4(48.0f, 2.0f, 40.0f)));
  world.push_back(std::move(bush));

  auto bush2 = createMesh("bush06.obj", "textures/bush6_spring5.png");
  bush2->applyTranslate(translate(
      Vector4(-bush2->centroid.x, -bush2->centroid.y, -bush2->centroid.z)));
  bush2->applyScale(scale(Vector4(4.0f, 4.0f, 4.0f)));
  float bush2_half_height = (bush2->aabb.max_y - bush2->aabb.min_y) / 2.0f;
  bush2->applyTranslate(translate(Vector4(48.0f, 2.0f, 65.0f)));
  world.push_back(std::move(bush2));

#pragma region road strips
  auto road_strip1 = createMesh("cube.obj", "");
  road_strip1->applyTranslate(
      translate(Vector4(-road_strip1->centroid.x, -road_strip1->centroid.y,
                        -road_strip1->centroid.z)));
  road_strip1->applyScale(scale(Vector4(3.0f, 0.01f, 1.0f)));
  road_strip1->applyTranslate(
      translate(Vector4(30.0f, half_road_height + 0.01f, 50.0f)));

  for (auto &face : road_strip1->faces)
  {
    face->color = Point3(1.0f, 1.0f, 1.0f);
    face->dif_color = Point3(1.0f, 1.0f, 1.0f);
  }

  auto road_strip2 = createMesh("cube.obj", "");
  road_strip2->applyTranslate(
      translate(Vector4(-road_strip2->centroid.x, -road_strip2->centroid.y,
                        -road_strip2->centroid.z)));
  road_strip2->applyScale(scale(Vector4(3.0f, 0.01f, 1.0f)));
  road_strip2->applyTranslate(
      translate(Vector4(42.0f, half_road_height + 0.01f, 50.0f)));

  for (auto &face : road_strip2->faces)
  {
    face->color = Point3(1.0f, 1.0f, 1.0f);
    face->dif_color = Point3(1.0f, 1.0f, 1.0f);
  }

  auto road_strip3 = createMesh("cube.obj", "");
  road_strip3->applyTranslate(
      translate(Vector4(-road_strip3->centroid.x, -road_strip3->centroid.y,
                        -road_strip3->centroid.z)));
  road_strip3->applyScale(scale(Vector4(3.0f, 0.01f, 1.0f)));
  road_strip3->applyTranslate(
      translate(Vector4(54.0f, half_road_height + 0.01f, 50.0f)));

  for (auto &face : road_strip3->faces)
  {
    face->color = Point3(1.0f, 1.0f, 1.0f);
    face->dif_color = Point3(1.0f, 1.0f, 1.0f);
  }

  auto road_strip4 = createMesh("cube.obj", "");
  road_strip4->applyTranslate(
      translate(Vector4(-road_strip4->centroid.x, -road_strip4->centroid.y,
                        -road_strip4->centroid.z)));
  road_strip4->applyScale(scale(Vector4(3.0f, 0.01f, 1.0f)));
  road_strip4->applyTranslate(
      translate(Vector4(66.0f, half_road_height + 0.01f, 50.0f)));

  for (auto &face : road_strip4->faces)
  {
    face->color = Point3(1.0f, 1.0f, 1.0f);
    face->dif_color = Point3(1.0f, 1.0f, 1.0f);
  }

  auto road_strip5 = createMesh("cube.obj", "");
  road_strip5->applyTranslate(
      translate(Vector4(-road_strip5->centroid.x, -road_strip5->centroid.y,
                        -road_strip5->centroid.z)));
  road_strip5->applyScale(scale(Vector4(3.0f, 0.01f, 1.0f)));
  road_strip5->applyTranslate(
      translate(Vector4(18.0f, half_road_height + 0.01f, 50.0f)));

  for (auto &face : road_strip5->faces)
  {
    face->color = Point3(1.0f, 1.0f, 1.0f);
    face->dif_color = Point3(1.0f, 1.0f, 1.0f);
  }

  auto road_strip6 = createMesh("cube.obj", "");
  road_strip6->applyTranslate(
      translate(Vector4(-road_strip6->centroid.x, -road_strip6->centroid.y,
                        -road_strip6->centroid.z)));
  road_strip6->applyScale(scale(Vector4(3.0f, 0.01f, 1.0f)));
  road_strip6->applyTranslate(
      translate(Vector4(6.0f, half_road_height + 0.01f, 50.0f)));

  for (auto &face : road_strip6->faces)
  {
    face->color = Point3(1.0f, 1.0f, 1.0f);
    face->dif_color = Point3(1.0f, 1.0f, 1.0f);
  }
#pragma endregion

  auto post_base = std::make_unique<Cylinder>(
      Point4(15.0f, 0.0f, 35.0f), 15.0f, 1.0f, Vector4(0.0f, 1.0f, 0.0f), true,
      true, post_color.color, post_color.color, post_color.spec);
  auto post_arm = std::make_unique<Cylinder>(
      Point4(15.0f, 16.0f, 34.0f), 6.0f, 1.0f, Vector4(0.0f, 0.0f, 1.0f), true,
      true, post_color.color, post_color.color, post_color.spec);
  auto lamp = std::make_unique<Sphere>(Point4(15.0f, 16.0f, 41.0f), 1.0f,
                                       lamp_color.color, lamp_color.color,
                                       lamp_color.spec);

  auto post_base2 = std::make_unique<Cylinder>(
      Point4(50.0f, 0.0f, 35.0f), 15.0f, 1.0f, Vector4(0.0f, 1.0f, 0.0f), true,
      true, post_color.color, post_color.color, post_color.spec);
  auto post_arm2 = std::make_unique<Cylinder>(
      Point4(50.0f, 16.0f, 35.0f), 6.0f, 1.0f, Vector4(0.0f, 0.0f, 1.0f), true,
      true, post_color.spec, post_color.spec, post_color.spec);
  auto lamp2 = std::make_unique<Sphere>(Point4(50.0f, 16.0f, 41.0f), 1.0f,
                                        lamp_color.color, lamp_color.color,
                                        lamp_color.spec);
#pragma region creto

  auto creto = createMesh("Hip Hop Dancing.obj", "textures/Old man.png");
  creto->applyScale(scale(Vector4(.05f, .05f, .05f)));
  creto->applyTranslate(translate(
      Vector4(25.f, 0.f, 30.f)));
  ListMesh *ptr_creto = creto.get();

#pragma region road cones
  auto road_cone1 = std::make_unique<Cone>(
      Point4(30.0f, 0.0f, 40.0f), .8f, true, Point4(30.0f, 2.0f, 40.0f),
      road_cone_color.color, road_cone_color.color, road_cone_color.spec);

  auto road_cone_base1 = createMesh("cube.obj", "");
  road_cone_base1->applyTranslate(translate(
      Vector4(-road_cone_base1->centroid.x, -road_cone_base1->centroid.y,
              -road_cone_base1->centroid.z)));
  road_cone_base1->applyScale(scale(Vector4(1.0f, 0.02f, 1.0f)));
  float half_base_height =
      (road_cone_base1->aabb.max_y - road_cone_base1->aabb.min_y) / 2.0f;
  road_cone_base1->applyTranslate(
      translate(Vector4(30.0f, half_base_height, 40.0f)));

  for (auto &face : road_cone_base1->faces)
  {
    face->color = road_cone_color.color;
    face->dif_color = road_cone_color.color;
    face->spec_color = road_cone_color.spec;
  }

  auto road_cone2 = std::make_unique<Cone>(
      Point4(35.0f, 0.0f, 40.0f), .8f, true, Point4(35.0f, 2.0f, 40.0f),
      road_cone_color.color, road_cone_color.color, road_cone_color.spec);

  auto road_cone_base2 = createMesh("cube.obj", "");
  road_cone_base2->applyTranslate(translate(
      Vector4(-road_cone_base2->centroid.x, -road_cone_base2->centroid.y,
              -road_cone_base2->centroid.z)));
  road_cone_base2->applyScale(scale(Vector4(1.0f, 0.02f, 1.0f)));
  road_cone_base2->applyTranslate(
      translate(Vector4(35.0f, half_base_height, 40.0f)));

  for (auto &face : road_cone_base2->faces)
  {
    face->color = road_cone_color.color;
    face->dif_color = road_cone_color.color;
    face->spec_color = road_cone_color.spec;
  }

  auto road_cone3 = std::make_unique<Cone>(
      Point4(25.0f, 0.0f, 40.0f), .8f, true, Point4(25.0f, 2.0f, 40.0f),
      road_cone_color.color, road_cone_color.color, road_cone_color.spec);

  auto road_cone_base3 = createMesh("cube.obj", "");
  road_cone_base3->applyTranslate(translate(
      Vector4(-road_cone_base3->centroid.x, -road_cone_base3->centroid.y,
              -road_cone_base3->centroid.z)));
  road_cone_base3->applyScale(scale(Vector4(1.0f, 0.02f, 1.0f)));
  road_cone_base3->applyTranslate(
      translate(Vector4(25.0f, half_base_height, 40.0f)));

  for (auto &face : road_cone_base3->faces)
  {
    face->color = road_cone_color.color;
    face->dif_color = road_cone_color.color;
    face->spec_color = road_cone_color.spec;
  }
#pragma endregion

#pragma region  trashes
  auto taxi = createMesh("Taxi.obj","textures/Taxi 256x256.png");
  taxi->applyScale(scale(Vector4(3.8f,3.8f,3.8f)));
  taxi->applyRotation(rotate(Vector4(0.f,1.f,0.f), 90.0f * M_PI / 180.0f ));
  taxi->applyTranslate(translate(Vector4(25.f,0.f,55.f)));

  world.push_back(std::move(car1));
  world.push_back(std::move(car2));
  world.push_back(std::move(car3));
  world.push_back(std::move(car4));
  world.push_back(std::move(shop));
  world.push_back(std::move(vendinha));
  world.push_back(std::move(japanesebuilding));
  world.push_back(std::move(road));
  world.push_back(std::move(road_strip1));
  world.push_back(std::move(road_strip2));
  world.push_back(std::move(road_strip3));
  world.push_back(std::move(road_strip4));
  world.push_back(std::move(road_strip5));
  world.push_back(std::move(road_strip6));
  world.push_back(std::move(post_base));
  world.push_back(std::move(post_arm));
  world.push_back(std::move(lamp));
  world.push_back(std::move(post_base2));
  world.push_back(std::move(post_arm2));
  world.push_back(std::move(lamp2));
  world.push_back(std::move(road_cone1));
  world.push_back(std::move(road_cone2));
  world.push_back(std::move(road_cone3));
  world.push_back(std::move(road_cone_base1));
  world.push_back(std::move(road_cone_base2));
  world.push_back(std::move(road_cone_base3));
  world.push_back(std::move(creto));
  world.push_back(std::move(taxi));

#pragma endregion

#pragma region plains
  Point3 specular_plains(.1, .1, .1);
  Point3 floor_col(.9, .5, 0);

  Point4 mirror_origin(18.0f, 0.01f, 15.f);
  Point4 mirror_width_pt(42.8f, 0.01f, 15.f);
  Point4 mirror_height_pt(18.0f, 10.0f, 15.f);

  Point3 mirror_color(0, 0, 0);
  Point3 mirror_spec(1, 1, 1);

  auto vitrine_espelhada = std::make_unique<Rectangle>(
      mirror_origin, mirror_width_pt, mirror_height_pt, mirror_color,
      mirror_color, mirror_spec, 1.0f);

//  world.push_back(std::move(vitrine_espelhada));
#pragma endregion

#pragma endregion

#pragma endregion

  float fov_atual = 53.0f;
  dWindow = calculate_dWindow_from_FOV(fov_atual, wWindow);

  w = (lookFrom - lookAt);
  w.normalize();
  u = cross(vUp, w);
  u.normalize();
  v_cam = cross(w, u);

  const int screenWidth = 800;
  const int screenHeight = 600;
  InitWindow(screenWidth, screenHeight, "Raycasting CG - Raylib Ativado");
  // gladLoadGL();
  // glViewport(0,0,screenWidth,screenHeight);
  for (const auto &obj : world)
  {
    // check if the object is a mesh
    ListMesh *mesh = dynamic_cast<ListMesh *>(obj.get());
    if (mesh && mesh->indices.size() > 1)
    {
      mesh->InitBuffers();
    }
  }

  Image rayImage = GenImageColor(screenWidth, screenHeight, BLACK);
  Texture2D tex = LoadTextureFromImage(rayImage);

  bool redraw = true;
  bool useRasterization = false;
  SetTargetFPS(60);

  while (!WindowShouldClose())
  {
    // rasterization toggle on/off
    if (IsKeyPressed(KEY_SPACE))
    {
      useRasterization = !useRasterization;
      redraw = true;
    }

    // picking
    if (!useRasterization && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
      float mouseX = GetMouseX();
      float mouseY = GetMouseY();
      float ndc_x, ndc_y;

      convertDisplayToWindow(mouseX, mouseY, ndc_x, ndc_y, xmin, xmax, ymin,
                             ymax, nCol, nLin);

      Vector4 ray_dir;
      if (projectionType == Projection::Perspective)
      {
        ray_dir = (u * ndc_x) + (v_cam * ndc_y) - (w * dWindow);
      }
      else
      {
        ray_dir = -w;
      }
      ray_dir.normalize();

      float closest_t = 99999.0f;
      HitRecord rec;
      Object *hit_obj = nullptr;

      for (const auto &obj : world)
      {
        HitRecord temp_rec;
        if (obj->Intersect(lookFrom, ray_dir, 0.001f, closest_t, temp_rec))
        {
          closest_t = temp_rec.t;
          hit_obj = obj.get();
        }
      }

      if (hit_obj)
      {
        selectedObject = hit_obj;
      }
      else
      {
        selectedObject = nullptr;
      }
    }

    // moving selected object with arrow keys
    if (selectedObject != nullptr)
    {
      float step = 1.0f;
      Vector4 moveVec(0, 0, 0, 0);
      bool movedObject = false;

      if (IsKeyDown(KEY_UP))
      {
        moveVec.z -= step;
        movedObject = true;
      }
      if (IsKeyDown(KEY_DOWN))
      {
        moveVec.z += step;
        movedObject = true;
      }
      if (IsKeyDown(KEY_LEFT))
      {
        moveVec.x -= step;
        movedObject = true;
      }
      if (IsKeyDown(KEY_RIGHT))
      {
        moveVec.x += step;
        movedObject = true;
      }

      if (movedObject)
      {
        ListMesh *mesh = dynamic_cast<ListMesh *>(selectedObject);
        if (mesh)
        {
          mesh->applyTranslate(translate(moveVec));
          redraw = true;
        }
      }
    }

    // camera control
    if (IsKeyPressed(KEY_ONE))
    {
      projectionType = Projection::Perspective;
      redraw = true;
    }
    if (IsKeyPressed(KEY_TWO))
    {
      projectionType = Projection::Ortographic;
      redraw = true;
    }
    if (IsKeyPressed(KEY_THREE))
    {
      projectionType = Projection::Oblique;
      redraw = true;
    }

    if (IsKeyDown(KEY_W))
    {
      lookFrom.z -= 1.0f;
      redraw = true;
    }
    if (IsKeyDown(KEY_S))
    {
      lookFrom.z += 1.0f;
      redraw = true;
    }
    if (IsKeyDown(KEY_A))
    {
      lookFrom.x -= 1.0f;
      redraw = true;
    }
    if (IsKeyDown(KEY_D))
    {
      lookFrom.x += 1.0f;
      redraw = true;
    }

    // focal distance control
    if (IsKeyDown(KEY_E))
    {
      dWindow += 0.2f;
      fov_atual = calculate_FOV_from_dWindow(dWindow, wWindow);
      redraw = true;
    }
    if (IsKeyDown(KEY_Q))
    {
      dWindow -= 0.2f;
      if (dWindow < 0.1f)
        dWindow = 0.1f;
      fov_atual = calculate_FOV_from_dWindow(dWindow, wWindow);
      redraw = true;
    }

    // fov adjust
    if (IsKeyDown(KEY_X))
    { // zoom out
      fov_atual += 1.0f;
      if (fov_atual > 160.0f)
        fov_atual = 160.0f;
      dWindow = calculate_dWindow_from_FOV(fov_atual, wWindow);
      redraw = true;
    }
    if (IsKeyDown(KEY_Z))
    { // zoom in
      fov_atual -= 1.0f;
      if (fov_atual < 10.0f)
        fov_atual = 10.0f;
      dWindow = calculate_dWindow_from_FOV(fov_atual, wWindow);
      redraw = true;
    }

    // camera sync
    w = (lookFrom - lookAt);
    w.normalize();
    u = cross(vUp, w);
    u.normalize();
    v_cam = cross(w, u);

    if (!useRasterization && redraw)
    {
      float oblique_scale = 0.5f;
      float oblique_angle_rad = 0.0f;

      for (int y = 0; y < nLin; y++)
      {
        for (int x = 0; x < nCol; x++)
        {
          float ndc_x, ndc_y;
          convertDisplayToWindow(x, y, ndc_x, ndc_y, xmin, xmax, ymin, ymax,
                                 nCol, nLin);

          Point4 ray_origin;
          Vector4 ray_dir;

          if (projectionType == Projection::Perspective)
          {
            ray_origin = lookFrom;
            ray_dir = (u * ndc_x) + (v_cam * ndc_y) - (w * dWindow);
          }
          else if (projectionType == Projection::Ortographic)
          {
            ray_origin = lookFrom + (u * ndc_x) + (v_cam * ndc_y);
            ray_dir = -w;
          }
          else
          {
            ray_origin = lookFrom + (u * ndc_x) + (v_cam * ndc_y);
            float s_x = oblique_scale * std::cos(oblique_angle_rad);
            float s_y = oblique_scale * std::sin(oblique_angle_rad);
            ray_dir = -w + (u * s_x) + (v_cam * s_y);
          }
          ray_dir.normalize();

          Point3 color =
              cast_ray(ray_origin, ray_dir, 2, world, lights, amb_light);

          if (edge_detection)
          {
            // nCol -> image width
            pixels[y * nCol + x] = color;
          }
          else
          {
            ::Color rlColor = {
                (unsigned char)(std::clamp(color.x, 0.0f, 1.0f) * 255),
                (unsigned char)(std::clamp(color.y, 0.0f, 1.0f) * 255),
                (unsigned char)(std::clamp(color.z, 0.0f, 1.0f) * 255), 255};

            ImageDrawPixel(&rayImage, x, y, rlColor);
          }
        }
      }
      if (edge_detection)
      {
        for (int y = 0; y < nLin; y++)
        {
          for (int x = 0; x < nCol; x++)
          {
            Point3 right_point = x + 1 < nCol ? pixels[y * nCol + (x + 1)] : Point3(0.0f, 0.0f, 0.0f);
            float right_px = x + 1 < nCol ? right_point.x * 0.299f + right_point.y * 0.587f + right_point.z * 0.114f : 0.0f;

            Point3 left_point = x - 1 >= 0 ? pixels[y * nCol + (x - 1)] : Point3(0.0f, 0.0f, 0.0f);
            float left_px = x - 1 >= 0 ? left_point.x * 0.299f + left_point.y * 0.587f + left_point.z * 0.114f : 0.0f;

            float gradient_x = (right_px - left_px) / 2.0f;

            Point3 down_point = y + 1 < nLin ? pixels[(y + 1) * nCol + x] : Point3(0.0f, 0.0f, 0.0f);
            float down_px = y + 1 < nLin ? down_point.x * 0.299f + down_point.y * 0.587f + down_point.z * 0.114f : 0.0f;

            Point3 up_point = y - 1 >= 0 ? pixels[(y - 1) * nCol + x] : Point3(0.0f, 0.0f, 0.0f);
            float up_px = y - 1 >= 0 ? up_point.x * 0.299f + up_point.y * 0.587f + up_point.z * 0.114f : 0.0f;

            float gradient_y = (down_px - up_px) / 2.0f;

            int grayscale = (int)(255.0f * sqrt(gradient_x * gradient_x + gradient_y * gradient_y));

            ::Color rlColor = {
                (unsigned char)grayscale,
                (unsigned char)grayscale,
                (unsigned char)grayscale, 255};

            ImageDrawPixel(&rayImage, x, y, rlColor);
          }
        }
      }
      UpdateTexture(tex, rayImage.data);
      redraw = false;
    }

    if (useRasterization)
    {
      ClearBackground(BLUE);

      // syncing raylib camera
      Camera3D glCamera = {0};
      glCamera.position = (RL_Vector3){lookFrom.x, lookFrom.y, lookFrom.z};
      glCamera.target = (RL_Vector3){lookAt.x, lookAt.y, lookAt.z};
      glCamera.up = (RL_Vector3){vUp.x, vUp.y, vUp.z};
      glCamera.fovy = fov_atual;
      glCamera.projection = CAMERA_PERSPECTIVE;

      //  BeginMode3D(glCamera);

      // DrawGrid(100, 5.0f); // Uma grade no chão (plano XZ)

      //  DrawCube((RL_Vector3){lookAt.x, lookAt.y, lookAt.z}, 2.0f, 2.0f, 2.0f, RED);
      //  DrawCubeWires((RL_Vector3){lookAt.x, lookAt.y, lookAt.z}, 2.0f, 2.0f, 2.0f, MAROON);

      //   DrawSphere((RL_Vector3){15.0f, 16.0f, 41.0f}, 1.0f, YELLOW);
      //   DrawSphere((RL_Vector3){50.0f, 16.0f, 41.0f}, 1.0f, YELLOW);
      Point4 centroidCreto = ptr_creto->centroid;
      ptr_creto->applyTranslate(translate(Vector4(-centroidCreto.x, -centroidCreto.y, -centroidCreto.z)));
      ptr_creto->applyRotation(rotate(Vector4(0.0f, 1.0f, 0.0f), .5f * M_PI / 180.0f));
      ptr_creto->applyTranslate(translate(Vector4(centroidCreto.x, centroidCreto.y, centroidCreto.z)));
      for (const auto &obj : world)
      {
        // check if the object is a mesh
        ListMesh *mesh = dynamic_cast<ListMesh *>(obj.get());
        if (mesh)
        {

          glm::vec3 glmPos = glm::vec3(lookFrom.x, lookFrom.y, lookFrom.z);

          glm::vec3 glmTarget = glm::vec3(lookAt.x, lookAt.y, lookAt.z);

          glm::vec3 glmUp = glm::vec3(vUp.x, vUp.y, vUp.z);

          glm::mat4 view = glm::lookAt(glmPos, glmTarget, glmUp);

          mesh->UpdateBuffers();
          mesh->Draw(view, fov_atual);
          BoundingBox box;
          box.min = (RL_Vector3){mesh->aabb.min_x, mesh->aabb.min_y, mesh->aabb.min_z};
          box.max = (RL_Vector3){mesh->aabb.max_x, mesh->aabb.max_y, mesh->aabb.max_z};

          DrawBoundingBox(box, GREEN);
        }
      }
      // EndMode3D();
    }
    else
    {
      ClearBackground(BLACK);
      DrawTexture(tex, 0, 0, WHITE);
    }

    // ui
    DrawRectangle(0, nLin - 85, nCol, 85, Fade(BLACK, 0.7f));

    if (useRasterization)
    {
      DrawText("MOTOR: RASTERIZACAO (GPU) - Aperte ESPACO para Raycasting", 10, nLin - 80, 20, GREEN);
    }
    else
    {
      DrawText("MOTOR: RAYCASTING (CPU) - Aperte ESPACO para OpenGL", 10, nLin - 80, 20, RED);
    }

    DrawText("1-3: Proj | W/A/S/D: Cam | Setas: Mover", 10, nLin - 55, 20, RAYWHITE);

    std::string text_focal = "Focal (Q/E): " + std::to_string(dWindow).substr(0, 4) +
                             "  |  FOV (Z/X): " + std::to_string((int)fov_atual) + " graus";
    DrawText(text_focal.c_str(), 10, nLin - 30, 20, YELLOW);

    DrawFPS(10, 10);
    EndDrawing();
  }

  // cleaning
  UnloadTexture(tex);
  UnloadImage(rayImage);
  CloseWindow();

  return 0;
}