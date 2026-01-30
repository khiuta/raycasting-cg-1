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

#define Vector3    RL_Vector3
#define Vector4    RL_Vector4
#define Matrix     RL_Matrix
#define Texture    RL_Texture
#define Rectangle  RL_Rectangle
#define Material   RL_Material

#include <raylib.h>

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

const float wWindow = 4.f, hWindow = 3.f;
const int nCol = wWindow*200, nLin = hWindow*200;
float dx = wWindow / nCol;
float dy = hWindow / nLin;
float dWindow = 4.0f;

float xmin = -2.0f, xmax = 2.0f;
float ymin = -1.5f, ymax = 1.5f;

enum class Projection{
  Perspective,
  Ortographic,
  Oblique
};
Projection projectionType = Projection::Perspective;

enum class LightType { DIRECTIONAL, SPOTLIGHT, POINTLIGHT };

struct Light {
  LightType type;
  Point4 position;
  Vector4 direction;
  Point3 color;
  float cutoff;
  float outer_cutoff;
};

struct Material {
  Point3 color;
  Point3 spec;
};

Object* selectedObject = nullptr;

std::vector<Light> lights;

Point3 amb_light(.3, .3, .3);
Point4 observer_pos(0, 0, 0);

Point4 lookFrom(45.0f, 15.0f, 85.0f);
Point4 lookAt(40.f, 5.0f, 50.0f);
Vector4 vUp(0.0f, 1.0f, 0.0f, 0.0f);
Vector4 u, v_cam, w;

std::vector<std::unique_ptr<Object>> world;

void convertDisplayToWindow(int display_x, int display_y, float &ndc_x, float& ndc_y) {
    float width_w = xmax - xmin;
    float height_w = ymax - ymin;
    float local_dx = width_w / nCol;
    float local_dy = height_w / nLin;
    ndc_x = xmin + local_dx/2.0f + (display_x * local_dx);
    ndc_y = ymax - local_dy/2.0f - (display_y * local_dy);
}

float hash(Vector4 v) {
    float d = dot(v, Vector4(12.9898, 78.233, 45.164, 9.456));
    return std::fmod(std::sin(d) * 43758.5453f, 1.0f);
}

Point3 getStarryBackground(const Vector4& dir) {
    float t = 0.5f * (dir.y + 1.0f);
    Point3 skyColor = (1.0f - t) * Point3(0.0, 0.0, 0.05) + t * Point3(0.02, 0.02, 0.1);
    float starIntensity = hash(dir);
    if (starIntensity > 0.996f) { 
        float sparkle = std::pow((starIntensity - 0.998f) / (1.0f - 0.998f), 4.0);
        return Point3(1.0, 1.0, 1.0) * sparkle;
    }
    return skyColor;
}
// 1. Função de Filtro Bilinear
Point3 sampleTextureBilinear(const Texture* tex, float u, float v) {
    if (!tex || tex->colors.empty()) return Point3(1, 0, 1); 

    float x = u * tex->width - 0.5f;
    float y = v * tex->height - 0.5f;

    int x_floor = std::floor(x);
    int y_floor = std::floor(y);
    int x_ceil = x_floor + 1;
    int y_ceil = y_floor + 1;

    float w_x = x - x_floor;
    float w_y = y - y_floor;
    float w_x_inv = 1.0f - w_x;
    float w_y_inv = 1.0f - w_y;

    auto getColor = [&](int cx, int cy) -> Point3 {
        cx = std::clamp(cx, 0, tex->width - 1);
        cy = std::clamp(cy, 0, tex->height - 1);
        auto pixel = tex->colors[cy][cx];
        return Point3(
            std::get<0>(pixel) / 255.0f,
            std::get<1>(pixel) / 255.0f,
            std::get<2>(pixel) / 255.0f
        );
    };

    Point3 c00 = getColor(x_floor, y_floor);
    Point3 c10 = getColor(x_ceil, y_floor);
    Point3 c01 = getColor(x_floor, y_ceil);
    Point3 c11 = getColor(x_ceil, y_ceil);

    Point3 top = Point3(
        c00.x * w_x_inv + c10.x * w_x,
        c00.y * w_x_inv + c10.y * w_x,
        c00.z * w_x_inv + c10.z * w_x
    );

    Point3 bottom = Point3(
        c01.x * w_x_inv + c11.x * w_x,
        c01.y * w_x_inv + c11.y * w_x,
        c01.z * w_x_inv + c11.z * w_x
    );

    return Point3(
        top.x * w_y_inv + bottom.x * w_y,
        top.y * w_y_inv + bottom.y * w_y,
        top.z * w_y_inv + bottom.z * w_y
    );
}

// 2. Função Auxiliar de Reflexão (MOVIDA PARA CIMA)
Vector4 reflect_ray(const Vector4& v, const Vector4& n) {
    return v - n * 2.0f * dot(v, n);
}

// 3. Função setColor (Agora ela conhece 'reflect_ray' e 'sampleTextureBilinear')
Point3 setColor(const Vector4 &d, HitRecord rec, std::vector<Light> lights){
  Point3 obj_color = rec.obj_ptr->getColor();

  if(rec.texture != nullptr && !rec.texture->colors.empty()) {
    float u = rec.uv.x;
    float v = rec.uv.y;
    u = u - std::floor(u);
    v = v - std::floor(v);
    v = 1.0f - v;

    // Usando o filtro bilinear
    obj_color = sampleTextureBilinear(rec.texture, u, v);
  }
  Point3 final_color = obj_color * amb_light;

  for (const auto& l : lights) {
    Vector4 light_dir;
    float dist_to_light;
    float intensity = 1.0f;

    if (l.type == LightType::DIRECTIONAL) {
      light_dir = -l.direction; 
      light_dir.normalize();
      dist_to_light = 1e6f; 
    } 
    else if (l.type == LightType::POINTLIGHT || l.type == LightType::SPOTLIGHT) {
      Vector4 L = l.position - rec.p_int;
      dist_to_light = L.length();
      light_dir = L / dist_to_light;

      if (l.type == LightType::SPOTLIGHT) {
        float theta = dot(l.direction, -light_dir);
        float epsilon = l.cutoff - l.outer_cutoff;
        intensity = std::clamp((theta - l.outer_cutoff) / epsilon, 0.0f, 1.0f);
      }
    }

    if (intensity <= 0.0f) continue;

    bool on_shadow = false;
    for (const auto& other : world) {
      if (other.get() == rec.obj_ptr) continue;
      HitRecord temp_rec;
      if (other->Intersect(rec.p_int, light_dir, 0.001f, dist_to_light, temp_rec)) {
        on_shadow = true;
        break;
      }
    }

    if (!on_shadow) {
      float dif_i = std::max(0.f, dot(rec.normal, light_dir)) * intensity;
      Point3 diff_part = (obj_color * l.color) * dif_i; 
      final_color = final_color + diff_part;

      // Agora funciona porque reflect_ray foi declarada antes
      Vector4 reflection = reflect_ray(rec.normal, light_dir);
      float spec_i = std::pow(std::max(0.f, dot(reflection, -d)), 50) * intensity;
      Point3 spec_part = (rec.obj_ptr->getSpecular() * l.color) * spec_i;
      final_color = final_color + spec_part;
    }
  }

  final_color.clamp();
  return final_color;
}
Point3 cast_ray(const Point4& ray_origin, const Vector4& ray_dir, int depth) {
    if (depth <= 0) return Point3(0,0,0);

    float closest_so_far = 99999.0f;
    HitRecord rec;
    bool hit_anything = false;
      
    for(const auto& object : world){
      HitRecord temp_rec;
      // t_min slightly greater than 0 to avoid acne
      if(object->Intersect(ray_origin, ray_dir, 0.001f, closest_so_far, temp_rec)){
        hit_anything = true;
        closest_so_far = temp_rec.t;
        rec = temp_rec;
      }
    }

    if(hit_anything){
        Point3 local_color = setColor(ray_dir, rec, lights);

        float reflectivity = rec.obj_ptr->getReflectivity();

        if (reflectivity > 0.0f) {
            Vector4 reflected_dir = reflect_ray(ray_dir, rec.normal);
            reflected_dir.normalize();

            Point3 reflected_color = cast_ray(rec.p_int, reflected_dir, depth - 1);

            return Point3(
                local_color.x * (1.0f - reflectivity) + reflected_color.x * reflectivity,
                local_color.y * (1.0f - reflectivity) + reflected_color.y * reflectivity,
                local_color.z * (1.0f - reflectivity) + reflected_color.z * reflectivity
            );
        }

        return local_color;
    } else {
        return getStarryBackground(ray_dir);
    }
}

void raycast(std::ofstream &image, int lin_start, int col_start, int width, int height) {
  float oblique_scale = 0.5f; 
  float oblique_angle_rad = 0.0f * (3.14159f / 180.0f);

  for(int l = lin_start; l < height; l++){
    for(int c = col_start; c < width; c++){
      float x, y;
      convertDisplayToWindow(c, l, x, y);

      Point4 ray_origin;
      Vector4 ray_dir;

      if (projectionType == Projection::Perspective) {
          ray_origin = lookFrom;
          ray_dir = (u * x) + (v_cam * y) - (w * dWindow);
          ray_dir.normalize();
      } 
      else if (projectionType == Projection::Ortographic) {
          ray_origin = lookFrom + (u * x) + (v_cam * y);
          ray_dir = -w; 
          ray_dir.normalize();
      }
      else if (projectionType == Projection::Oblique) {
          ray_origin = lookFrom + (u * x) + (v_cam * y);
          float shear_x = oblique_scale * std::cos(oblique_angle_rad);
          float shear_y = oblique_scale * std::sin(oblique_angle_rad);
          ray_dir = -w + (u * shear_x) + (v_cam * shear_y);
          ray_dir.normalize();
      }

      Point3 final_color = cast_ray(ray_origin, ray_dir, 3);

      int r_int = (int)(final_color.x * 255);
      int g_int = (int)(final_color.y * 255);
      int b_int = (int)(final_color.z * 255);
      
      r_int = std::clamp(r_int, 0, 255);
      g_int = std::clamp(g_int, 0, 255);
      b_int = std::clamp(b_int, 0, 255);

      image << r_int << " " << g_int << " " << b_int << " ";
    }
    image << "\n";
  }
}

void fill_xyz(std::string line, float &x, float &y, float &z){
  bool is_negative = false;
  int control = 0; 
  for(int i = 2; i < line.size(); i++){
    if(line[i] == '-') is_negative = true;
    else if(line[i] >= 48 && line[i] <= 57){
      int k = i;
      std::string digit;
      if(is_negative) digit += '-';
      while(line[k] >= 48 && line[k] <= 57 || line[k] == '.'){
        digit += line[k];
        k++;
      }
      i += k-i;
      is_negative = false;
      if(control == 0) x = std::stof(digit);
      else if(control == 1) y = std::stof(digit);
      else if(control == 2) z = std::stof(digit);
      control++;
    }
  }
}

void read_obj_file(const std::string& filename,
                   std::vector<std::unique_ptr<Point4>> &v,
                   std::vector<std::unique_ptr<Vector4>> &vn,
                   std::vector<std::unique_ptr<Point3>> &vt,
                   std::vector<std::unique_ptr<Triangle>> &f,
                   Point4 &centroid,
                   AABB &aabb,
                   ListMesh *mesh){
    std::ifstream file(filename);
    if(!file.is_open()) {
        std::cerr << "ERROR: There was a problem opening the file " << filename << ".\n";
        return;
    }
    std::string line;
    float max_x, min_x, max_y, min_y, max_z, min_z;
    bool first_vertice = true;
    while(std::getline(file, line)){
        if(line[0] == 'v'){
            if(line[1] == ' '){
                float x, y, z;
                fill_xyz(line, x, y, z);
                if(first_vertice){ min_x = max_x = x; min_y = max_y = y; min_z = max_z = z; first_vertice = false; } 
                else {
                    if(x < min_x) min_x = x; if(x > max_x) max_x = x;
                    if(y < min_y) min_y = y; if(y > max_y) max_y = y;
                    if(z < min_z) min_z = z; if(z > max_z) max_z = z;
                }
                v.push_back(std::make_unique<Point4>(x, y, z, 1));
            } else if(line[1] == 'n'){
                float x, y, z; fill_xyz(line, x, y, z);
                vn.push_back(std::make_unique<Vector4>(x, y, z, 0));
            } else if(line[1] == 't'){
                float x, y, z = 0; fill_xyz(line, x, y, z);
                vt.push_back(std::make_unique<Point3>(x, y, z));
            }
        } else if(line[0] == 'f'){
            int point_control = 0; int vertex_control = 0;
            int vertex_indices[4] = {0, 0, 0, -1};
            int tex_vertex_indices[4] = {0, 0, 0, 0};
            int nor_vertex_indices[4] = {0, 0, 0, 0};
            for(int i = 2; i < line.size(); i++){
                if(line[i] == ' ') { point_control++; vertex_control = 0; }
                if(line[i] == '/') vertex_control++;
                if(line[i] >= 48 && line[i] <= 57){
                    int k = i; std::string digit;
                    while(line[k] >= 48 && line[k] <= 57){ digit += line[k]; k++; }
                    if(vertex_control == 0) vertex_indices[point_control] = std::stoi(digit) - 1;
                    else if(vertex_control == 1) tex_vertex_indices[point_control] = std::stoi(digit) - 1;
                    else if(vertex_control == 2) nor_vertex_indices[point_control] = std::stoi(digit) - 1;
                    i = k - 1;
                }
            }
            if(vertex_indices[3] != -1){
                Point4 p1 = *v[vertex_indices[0]]; Point4 p2 = *v[vertex_indices[1]]; Point4 p3 = *v[vertex_indices[2]]; Point4 p4 = *v[vertex_indices[3]];
                Vector4 normal = *vn[nor_vertex_indices[0]];
                if(vt.size() > 0){
                    Point3 vt1 = *vt[tex_vertex_indices[0]]; Point3 vt2 = *vt[tex_vertex_indices[1]]; Point3 vt3 = *vt[tex_vertex_indices[2]]; Point3 vt4 = *vt[tex_vertex_indices[3]];
                    auto new_tri_1 = std::make_unique<Triangle>(p1, p2, p3, normal, vt1, vt2, vt3);
                    auto new_tri_2 = std::make_unique<Triangle>(p1, p3, p4, normal, vt1, vt3, vt4);
                    new_tri_1->SetMesh(mesh); new_tri_2->SetMesh(mesh);
                    Triangle* tp1 = new_tri_1.get(); Triangle* tp2 = new_tri_2.get();
                    f.push_back(std::move(new_tri_1)); f.push_back(std::move(new_tri_2));
                    aabb.t.push_back(tp1); aabb.t.push_back(tp2);
                } else {
                    auto new_tri_1 = std::make_unique<Triangle>(p1, p2, p3, normal); auto new_tri_2 = std::make_unique<Triangle>(p1, p3, p4, normal);
                    new_tri_1->SetMesh(mesh); new_tri_2->SetMesh(mesh);
                    Triangle* tp1 = new_tri_1.get(); Triangle* tp2 = new_tri_2.get();
                    f.push_back(std::move(new_tri_1)); f.push_back(std::move(new_tri_2));
                    aabb.t.push_back(tp1); aabb.t.push_back(tp2);
                }
            } else {
                Point4 p1 = *v[vertex_indices[0]]; Point4 p2 = *v[vertex_indices[1]]; Point4 p3 = *v[vertex_indices[2]];
                Vector4 normal = *vn[nor_vertex_indices[0]];
                if(vt.size() > 0){
                    Point3 vt1 = *vt[tex_vertex_indices[0]]; Point3 vt2 = *vt[tex_vertex_indices[1]]; Point3 vt3 = *vt[tex_vertex_indices[2]];
                    auto new_tri = std::make_unique<Triangle>(p1, p2, p3, normal, vt1, vt2, vt3);
                    new_tri->SetMesh(mesh); aabb.t.push_back(new_tri.get()); f.push_back(std::move(new_tri));
                } else {
                    auto new_tri = std::make_unique<Triangle>(p1, p2, p3, normal);
                    new_tri->SetMesh(mesh); aabb.t.push_back(new_tri.get()); f.push_back(std::move(new_tri));
                } 
            }
        }
    }
    centroid.x = (max_x + min_x)/2; centroid.y = (max_y + min_y)/2; centroid.z = (max_z + min_z)/2;
    aabb.min_x = min_x; aabb.max_x = max_x; aabb.min_y = min_y; aabb.max_y = max_y; aabb.min_z = min_z; aabb.max_z = max_z;
}

float random_float2() {
  static std::mt19937 generator(std::chrono::high_resolution_clock::now().time_since_epoch().count());
  static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
  return distribution(generator);
}

std::unique_ptr<ListMesh> createMesh(const std::string& objPath, const std::string& texturePath) {
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

void handlePicking() {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        // gets the mouse position
        float mouseX = GetMouseX();
        float mouseY = GetMouseY();

        float ndc_x, ndc_y;
        // map the click to the window
        convertDisplayToWindow(mouseX, mouseY, ndc_x, ndc_y);

        // generate a ray based on the current projection
        Vector4 ray_dir = (u * ndc_x) + (v_cam * ndc_y) - (w * dWindow);
        ray_dir.normalize();

        float closest_t = 99999.0f;
        Object* hit_obj = nullptr;
        HitRecord rec;

        // check the world to see what was clicked
        for (const auto& obj : world) {
            HitRecord temp_rec;
            if (obj->Intersect(lookFrom, ray_dir, 0.001f, closest_t, temp_rec)) {
                closest_t = temp_rec.t;
                hit_obj = obj.get();
            }
        }

        // verify if the hit object is a mesh (mathematically defined objects still dont have transformation)
        if (hit_obj) {
            selectedObject = hit_obj;
            std::cout << "Objeto selecionado!" << std::endl;
        } else {
            selectedObject = nullptr;
        }
    }
}

int main() {
  std::string obj_name = "car_1.obj";

  Point3 spec = Point3(0.5f, 0.5f, 0.5f);
  Point3 low_spec = Point3(0.1f, 0.1f, 0.1f);

  Material lamp_color; lamp_color.color = Point3(1.0f, 0.9f, 0.5f); lamp_color.spec = spec;
  Material post_color; post_color.color = Point3(0.2f, 0.2f, 0.2f); post_color.spec = spec;
  Material road_cone_color; road_cone_color.color = Point3(0.8f, 0.4f, 0.1f); road_cone_color.spec = low_spec;
  Material road_strip_color; road_strip_color.color = Point3(1.0f, 1.0f, 1.0f); road_strip_color.spec = low_spec;

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

  lights.push_back(directional);
  lights.push_back(post_spot);
  lights.push_back(post_spot2);

  #pragma region world objects
  auto car1 = createMesh("car_1.obj", "textures/car_1.png");  

  car1->applyTranslate(translate(Vector4(-car1->centroid.x, -car1->centroid.y, -car1->centroid.z)));
  car1->applyScale(scale(Vector4(0.1, 0.1, 0.1)));
  Vector4 A_factors(0.5f, 0.0f, 0.0f);
  Vector4 B_factors(0.0f, 0.0f, 0.0f);
  car1->applyShear(shear(A_factors, B_factors));
  car1->applyRotation(rotate(Vector4(0.0f, 1.0f, 0.0f), -90.0f * M_PI / 180.0f));
  float car_half_height = (car1->aabb.max_y - car1->aabb.min_y) / 2.0f;
  car1->applyTranslate(translate(Vector4(20.0f, car_half_height, 30.0f)));


  auto car2 = createMesh("car_1.obj", "textures/car_1.png");

  car2->applyTranslate(translate(Vector4(-car2->centroid.x, -car2->centroid.y, -car2->centroid.z)));
  car2->applyScale(scale(Vector4(0.1f, 0.1f, 0.1f)));
  car2->applyRotation(rotate(Vector4(0.0f, 1.0f, 0.0f), -90.0f * M_PI / 180.0f));
  car2->applyTranslate(translate(Vector4(30.0f, car_half_height, 30.0f)));

  auto car3 = createMesh("car_1.obj", "textures/car_1.png");

  car3->applyTranslate(translate(Vector4(-car3->centroid.x, -car3->centroid.y, -car3->centroid.z)));
  car3->applyScale(scale(Vector4(0.1f, 0.1f, 0.1f)));
  car3->applyRotation(rotate(Vector4(0.0f, 1.0f, 0.0f), -90.0f * M_PI / 180.0f));
  car3->applyTranslate(translate(Vector4(40.0f, car_half_height, 30.0f)));

  auto cube = createMesh("cube.obj", "");
  cube->applyTranslate(translate(Vector4(-cube->centroid.x, -cube->centroid.y, -cube->centroid.z)));
  cube->applyScale(scale(Vector4(25.0f, 8.0f, 10.0f)));
  float half_cube_height = (cube->aabb.max_y - cube->aabb.min_y) / 2.0f;
  cube->applyTranslate(translate(Vector4(0.0f, half_cube_height, -25.0f)));
  for(auto& face : cube->faces){ face->reflectivity = 1.0f; }

  auto shop = createMesh("loja.obj", "textures/loja.png");
  shop->applyTranslate(translate(Vector4(-shop->centroid.x, -shop->centroid.y, -shop->centroid.z)));
  shop->applyScale(scale(Vector4(6.0f, 10.0f, 5.0f)));
  shop->applyRotation(rotate(Vector4(1.0f, 0.0f, 0.0f), 90.0f * M_PI / 180.0f));
  float half_shop_height = (shop->aabb.max_y - shop->aabb.min_y) / 2.0f;
  shop->applyTranslate(translate(Vector4(30.0f, half_shop_height, 5.0f)));
  //world.push_back(std::move(shop));


  auto road = createMesh("cube.obj", "");
  road->applyTranslate(translate(Vector4(-road->centroid.x, -road->centroid.y, -road->centroid.z)));
  road->applyScale(scale(Vector4(60.0f, 0.01f, 10.0f)));
  float half_road_height = (road->aabb.max_y - road->aabb.min_y) / 2.0f;
  road->applyTranslate(translate(Vector4(30.0f, half_road_height, 50.0f)));

  #pragma region vegetation

  auto sidewalk = createMesh("floor.obj", "textures/floor_texture.png");
  sidewalk->applyTranslate(translate(Vector4(-sidewalk->centroid.x, -sidewalk->centroid.y, -sidewalk->centroid.z)));
  sidewalk->applyScale(scale(Vector4(1.0f, 1.0f, 1.0f))); 
  sidewalk->applyRotation(rotate(Vector4(1.0f, 0.0f, 0.0f), 90.0f * M_PI / 180.0f));

  float sidewalk_half_height = (sidewalk->aabb.max_y - sidewalk->aabb.min_y) / 2.0f;
  sidewalk->applyTranslate(translate(Vector4(10.0f, -0.3f, 35.0f)));
  world.push_back(std::move(sidewalk));


  auto tree = createMesh("tree01.obj", "textures/tree01_spring.png");
  tree->applyTranslate(translate(Vector4(-tree->centroid.x, -tree->centroid.y, -tree->centroid.z)));
  tree->applyScale(scale(Vector4(4.0f, 4.0f, 4.0f))); 
  float tree_half_height = (tree->aabb.max_y - tree->aabb.min_y) / 2.0f;
  tree->applyTranslate(translate(Vector4(10.0f, 5.0f, 40.0f)));
  world.push_back(std::move(tree));

  auto bush = createMesh("bush01.obj", "textures/bush1_spring.png");
  bush->applyTranslate(translate(Vector4(-bush->centroid.x, -bush->centroid.y, -bush->centroid.z)));
  bush->applyScale(scale(Vector4(4.0f, 4.0f, 4.0f))); 
  float bush_half_height = (bush->aabb.max_y - bush->aabb.min_y) / 2.0f;
  bush->applyTranslate(translate(Vector4(48.0f, 2.0f, 40.0f)));
  world.push_back(std::move(bush));

  auto bush2 = createMesh("bush06.obj", "textures/bush6_spring5.png");
  bush2->applyTranslate(translate(Vector4(-bush2->centroid.x, -bush2->centroid.y, -bush2->centroid.z)));
  bush2->applyScale(scale(Vector4(4.0f, 4.0f, 4.0f))); 
  float bush2_half_height = (bush2->aabb.max_y - bush2->aabb.min_y) / 2.0f;
  bush2->applyTranslate(translate(Vector4(48.0f, 2.0f, 65.0f)));
  world.push_back(std::move(bush2));

  #pragma region road strips
  auto road_strip1 = createMesh("cube.obj", "");
  road_strip1->applyTranslate(translate(Vector4(-road_strip1->centroid.x, -road_strip1->centroid.y, -road_strip1->centroid.z)));
  road_strip1->applyScale(scale(Vector4(3.0f, 0.01f, 1.0f)));
  road_strip1->applyTranslate(translate(Vector4(30.0f, half_road_height + 0.01f, 50.0f)));

  for(auto& face : road_strip1->faces){
    face->color = Point3(1.0f, 1.0f, 1.0f);
    face->dif_color = Point3(1.0f, 1.0f, 1.0f);
  }
  
  auto road_strip2 = createMesh("cube.obj", "");
  road_strip2->applyTranslate(translate(Vector4(-road_strip2->centroid.x, -road_strip2->centroid.y, -road_strip2->centroid.z)));
  road_strip2->applyScale(scale(Vector4(3.0f, 0.01f, 1.0f)));
  road_strip2->applyTranslate(translate(Vector4(42.0f, half_road_height + 0.01f, 50.0f)));

  for(auto& face : road_strip2->faces){
    face->color = Point3(1.0f, 1.0f, 1.0f);
    face->dif_color = Point3(1.0f, 1.0f, 1.0f);
  }

  auto road_strip3 = createMesh("cube.obj", "");
  road_strip3->applyTranslate(translate(Vector4(-road_strip3->centroid.x, -road_strip3->centroid.y, -road_strip3->centroid.z)));
  road_strip3->applyScale(scale(Vector4(3.0f, 0.01f, 1.0f)));
  road_strip3->applyTranslate(translate(Vector4(54.0f, half_road_height + 0.01f, 50.0f)));

  for(auto& face : road_strip3->faces){
    face->color = Point3(1.0f, 1.0f, 1.0f);
    face->dif_color = Point3(1.0f, 1.0f, 1.0f);
  }

  auto road_strip4 = createMesh("cube.obj", "");
  road_strip4->applyTranslate(translate(Vector4(-road_strip4->centroid.x, -road_strip4->centroid.y, -road_strip4->centroid.z)));
  road_strip4->applyScale(scale(Vector4(3.0f, 0.01f, 1.0f)));
  road_strip4->applyTranslate(translate(Vector4(66.0f, half_road_height + 0.01f, 50.0f)));

  for(auto& face : road_strip4->faces){
    face->color = Point3(1.0f, 1.0f, 1.0f);
    face->dif_color = Point3(1.0f, 1.0f, 1.0f);
  }

  auto road_strip5 = createMesh("cube.obj", "");
  road_strip5->applyTranslate(translate(Vector4(-road_strip5->centroid.x, -road_strip5->centroid.y, -road_strip5->centroid.z)));
  road_strip5->applyScale(scale(Vector4(3.0f, 0.01f, 1.0f)));
  road_strip5->applyTranslate(translate(Vector4(18.0f, half_road_height + 0.01f, 50.0f)));

  for(auto& face : road_strip5->faces){
    face->color = Point3(1.0f, 1.0f, 1.0f);
    face->dif_color = Point3(1.0f, 1.0f, 1.0f);
  }

  auto road_strip6 = createMesh("cube.obj", "");
  road_strip6->applyTranslate(translate(Vector4(-road_strip6->centroid.x, -road_strip6->centroid.y, -road_strip6->centroid.z)));
  road_strip6->applyScale(scale(Vector4(3.0f, 0.01f, 1.0f)));
  road_strip6->applyTranslate(translate(Vector4(6.0f, half_road_height + 0.01f, 50.0f)));

  for(auto& face : road_strip6->faces){
    face->color = Point3(1.0f, 1.0f, 1.0f);
    face->dif_color = Point3(1.0f, 1.0f, 1.0f);
  }
  #pragma endregion

  auto post_base = std::make_unique<Cylinder>(Point4(15.0f, 0.0f, 35.0f), 15.0f, 1.0f, Vector4(0.0f, 1.0f, 0.0f), true, true, 
                                              post_color.color, 
                                              post_color.color,
                                              post_color.spec);
  auto post_arm = std::make_unique<Cylinder>(Point4(15.0f, 16.0f, 34.0f), 6.0f, 1.0f, Vector4(0.0f, 0.0f, 1.0f), true, true,
                                              post_color.color,
                                              post_color.color,
                                              post_color.spec);
  auto lamp = std::make_unique<Sphere>(Point4(15.0f, 16.0f, 41.0f), 1.0f,
                                        lamp_color.color,
                                        lamp_color.color,
                                        lamp_color.spec);

  auto post_base2 = std::make_unique<Cylinder>(Point4(50.0f, 0.0f, 35.0f), 15.0f, 1.0f, Vector4(0.0f, 1.0f, 0.0f), true, true, 
                                              post_color.color, 
                                              post_color.color,
                                              post_color.spec);
  auto post_arm2 = std::make_unique<Cylinder>(Point4(50.0f, 16.0f, 35.0f), 6.0f, 1.0f, Vector4(0.0f, 0.0f, 1.0f), true, true,
                                              post_color.spec,
                                              post_color.spec,
                                              post_color.spec);
  auto lamp2 = std::make_unique<Sphere>(Point4(50.0f, 16.0f, 41.0f), 1.0f,
                                        lamp_color.color,
                                        lamp_color.color,
                                        lamp_color.spec);
  
  #pragma region road cones
  auto road_cone1 = std::make_unique<Cone>(Point4(30.0f, 0.0f, 40.0f), 5.0f, true, Point4(30.0f, 2.0f, 40.0f),
                                          road_cone_color.color,
                                          road_cone_color.color,
                                          road_cone_color.spec);

  auto road_cone_base1 = createMesh("cube.obj", "");
  road_cone_base1->applyTranslate(translate(Vector4(-road_cone_base1->centroid.x, -road_cone_base1->centroid.y, -road_cone_base1->centroid.z)));
  road_cone_base1->applyScale(scale(Vector4(1.0f, 0.02f, 1.0f)));
  float half_base_height = (road_cone_base1->aabb.max_y - road_cone_base1->aabb.min_y) / 2.0f;
  road_cone_base1->applyTranslate(translate(Vector4(30.0f, half_base_height, 40.0f)));

  for(auto& face : road_cone_base1->faces){
    face->color = road_cone_color.color;
    face->dif_color = road_cone_color.color;
    face->spec_color = road_cone_color.spec;
  }

  auto road_cone2 = std::make_unique<Cone>(Point4(35.0f, 0.0f, 40.0f), 5.0f, true, Point4(35.0f, 2.0f, 40.0f),
                                          road_cone_color.color,
                                          road_cone_color.color,
                                          road_cone_color.spec);

  auto road_cone_base2 = createMesh("cube.obj", "");
  road_cone_base2->applyTranslate(translate(Vector4(-road_cone_base2->centroid.x, -road_cone_base2->centroid.y, -road_cone_base2->centroid.z)));
  road_cone_base2->applyScale(scale(Vector4(1.0f, 0.02f, 1.0f)));
  road_cone_base2->applyTranslate(translate(Vector4(35.0f, half_base_height, 40.0f)));

  for(auto& face : road_cone_base2->faces){
    face->color = road_cone_color.color;
    face->dif_color = road_cone_color.color;
    face->spec_color = road_cone_color.spec;
  }

  auto road_cone3 = std::make_unique<Cone>(Point4(25.0f, 0.0f, 40.0f), 5.0f, true, Point4(25.0f, 2.0f, 40.0f),
                                          road_cone_color.color,
                                          road_cone_color.color,
                                          road_cone_color.spec);

  auto road_cone_base3 = createMesh("cube.obj", "");
  road_cone_base3->applyTranslate(translate(Vector4(-road_cone_base3->centroid.x, -road_cone_base3->centroid.y, -road_cone_base3->centroid.z)));
  road_cone_base3->applyScale(scale(Vector4(1.0f, 0.02f, 1.0f)));
  road_cone_base3->applyTranslate(translate(Vector4(25.0f, half_base_height, 40.0f)));

  for(auto& face : road_cone_base3->faces){
    face->color = road_cone_color.color;
    face->dif_color = road_cone_color.color;
    face->spec_color = road_cone_color.spec;
  }
  #pragma endregion
  
  world.push_back(std::move(car1));
  world.push_back(std::move(car2));
  world.push_back(std::move(car3));
  //world.push_back(std::move(shop));
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
  #pragma endregion

  #pragma region plains
  Point3 specular_plains(.1, .1, .1);
  Point3 floor_col(.9, .5, 0);
  

  //world.push_back(std::make_unique<Plain>(Point4(0, 0, 0), Vector4(0, 1, 0), floor_col, floor_col, specular_plains));

  Point4 mirror_origin(16.0f, 0.01f, 20.1f);
  Point4 mirror_width_pt(46.0f, 0.01f, 20.1f);
  Point4 mirror_height_pt(16.0f, 10.0f, 20.1f); 

  Point3 mirror_color(0, 0, 0); 
  Point3 mirror_spec(1, 1, 1);

  auto vitrine_espelhada = std::make_unique<Rectangle>(
      mirror_origin,
      mirror_width_pt,
      mirror_height_pt,
      mirror_color, mirror_color, mirror_spec, 
      1.0f
  );

  world.push_back(std::move(vitrine_espelhada));

  #pragma endregion

  w = (lookFrom - lookAt); 
  w.normalize();
  u = cross(vUp, w); 
  u.normalize();
  v_cam = cross(w, u);

  const int screenWidth = 800;
  const int screenHeight = 600;
  InitWindow(screenWidth, screenHeight, "Raycasting CG - Raylib Ativado");

  Image rayImage = GenImageColor(screenWidth, screenHeight, BLACK);
  Texture2D tex = LoadTextureFromImage(rayImage);

  bool redraw = true; 

  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    // picking
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        float mouseX = GetMouseX();
        float mouseY = GetMouseY();
        float ndc_x, ndc_y;
        
        // map the click to window coordinates
        convertDisplayToWindow(mouseX, mouseY, ndc_x, ndc_y);

        Vector4 ray_dir;
        if (projectionType == Projection::Perspective) {
            ray_dir = (u * ndc_x) + (v_cam * ndc_y) - (w * dWindow);
        } else {
            ray_dir = -w;
        }
        ray_dir.normalize();

        float closest_t = 99999.0f;
        HitRecord rec;
        Object* hit_obj = nullptr;

        // test intersection with world
        for (const auto& obj : world) {
            HitRecord temp_rec;
            if (obj->Intersect(lookFrom, ray_dir, 0.001f, closest_t, temp_rec)) {
                closest_t = temp_rec.t;
                hit_obj = obj.get();
            }
        }

        // filter to select only meshes
        if (hit_obj) {
            selectedObject = hit_obj;
        } else {
            selectedObject = nullptr;
        }
    }

    // moving selected object with arrow keys
    if (selectedObject != nullptr) {
        float step = 1.0f;
        Vector4 moveVec(0, 0, 0, 0);
        bool movedObject = false;

        if (IsKeyDown(KEY_UP))    { moveVec.z -= step; movedObject = true; }
        if (IsKeyDown(KEY_DOWN))  { moveVec.z += step; movedObject = true; }
        if (IsKeyDown(KEY_LEFT))  { moveVec.x -= step; movedObject = true; }
        if (IsKeyDown(KEY_RIGHT)) { moveVec.x += step; movedObject = true; }

        if (movedObject) {
            // tries to convert to listmesh
            ListMesh* mesh = dynamic_cast<ListMesh*>(selectedObject);
            if (mesh) {
                mesh->applyTranslate(translate(moveVec));
                redraw = true; // forced re-rendering if the object moves
            }
        }
    }

    // keyboard input
    if (IsKeyPressed(KEY_ONE))   { projectionType = Projection::Perspective; redraw = true; }
    if (IsKeyPressed(KEY_TWO))   { projectionType = Projection::Ortographic; redraw = true; }
    if (IsKeyPressed(KEY_THREE)) { projectionType = Projection::Oblique;     redraw = true; }

    if (IsKeyDown(KEY_W)) { lookFrom.z -= 1.0f; redraw = true; }
    if (IsKeyDown(KEY_S)) { lookFrom.z += 1.0f; redraw = true; }
    if (IsKeyDown(KEY_A)) { lookFrom.x -= 1.0f; redraw = true; }
    if (IsKeyDown(KEY_D)) { lookFrom.x += 1.0f; redraw = true; }

    // rendering
    if (redraw) {
        w = (lookFrom - lookAt); w.normalize();
        u = cross(vUp, w); u.normalize();
        v_cam = cross(w, u);

        float oblique_scale = 0.5f; 
        float oblique_angle_rad = 0.0f;

        for (int y = 0; y < nLin; y++) {
            for (int x = 0; x < nCol; x++) {
                float ndc_x, ndc_y;
                convertDisplayToWindow(x, y, ndc_x, ndc_y);

                Point4 ray_origin;
                Vector4 ray_dir;

                if (projectionType == Projection::Perspective) {
                    ray_origin = lookFrom;
                    ray_dir = (u * ndc_x) + (v_cam * ndc_y) - (w * dWindow);
                } else if (projectionType == Projection::Ortographic) {
                    ray_origin = lookFrom + (u * ndc_x) + (v_cam * ndc_y);
                    ray_dir = -w;
                } else {
                    ray_origin = lookFrom + (u * ndc_x) + (v_cam * ndc_y);
                    float s_x = oblique_scale * std::cos(oblique_angle_rad);
                    float s_y = oblique_scale * std::sin(oblique_angle_rad);
                    ray_dir = -w + (u * s_x) + (v_cam * s_y);
                }
                ray_dir.normalize();

                Point3 color = cast_ray(ray_origin, ray_dir, 2); 

                ::Color rlColor = {
                    (unsigned char)(std::clamp(color.x, 0.0f, 1.0f) * 255),
                    (unsigned char)(std::clamp(color.y, 0.0f, 1.0f) * 255),
                    (unsigned char)(std::clamp(color.z, 0.0f, 1.0f) * 255),
                    255
                };
                ImageDrawPixel(&rayImage, x, y, rlColor);
            }
        }
        UpdateTexture(tex, rayImage.data);
        redraw = false;
    }

    BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(tex, 0, 0, WHITE);
        
        // ui
        DrawRectangle(0, nLin - 60, nCol, 60, Fade(BLACK, 0.6f));
        if (selectedObject) {
            DrawText("OBJETO SELECIONADO: Use as SETAS para mover", 10, nLin - 55, 20, GREEN);
        } else {
            DrawText("Clique em uma mesh para selecionar", 10, nLin - 55, 20, RAYWHITE);
        }
        DrawText("1-3: Proj | W/A/S/D: Cam", 10, nLin - 30, 20, RAYWHITE);
        
        DrawFPS(10, 10);
    EndDrawing();
}

  // cleaning
  UnloadTexture(tex);
  UnloadImage(rayImage);
  CloseWindow();

  return 0;

  // auto full_start = std::chrono::high_resolution_clock::now();
  // int frames = 1;
  
  // for(int i = 0; i < frames; i++){
  //     auto start = std::chrono::high_resolution_clock::now();

  //     std::string image_name = "frames/frame_";
  //     if(i < 10) image_name += "00";
  //     else if(i < 100) image_name += "0";
  //     image_name += std::to_string(i) + ".ppm";
      
  //     std::ofstream image(image_name);

  //     if(image.is_open()) {
  //         image << "P3\n" << nCol << " " << nLin << "\n255\n";
  //         raycast(image, 0, 0, nCol, nLin);
  //         image.close();
  //     }

  //     auto stop = std::chrono::high_resolution_clock::now();
  //     std::chrono::duration<double> elapsed = stop - start;
  //     std::cout << "Frame " << i+1 << " rendered in " << elapsed.count() << " seconds.\n";
  // }

  // auto full_stop = std::chrono::high_resolution_clock::now();
  // std::chrono::duration<double> elapsed_total = full_stop - full_start;
  // std::cout << "Total time: " << elapsed_total.count() << " seconds.\n";

  // return 0;
}