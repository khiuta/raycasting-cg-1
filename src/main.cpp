#include "../utils/Point4.hpp"
#include "../utils/Point3.hpp"
#include "../utils/Vector4.hpp"
#include "../utils/Triangle.hpp"
#include "../utils/Object.hpp"
#include "../utils/HitRecord.hpp"
#include "../utils/ListMesh.hpp"
#include "../utils/Plain.hpp"
#include "../utils/AABB.hpp"
#include "../utils/Sphere.hpp"
#include "../utils/Cylinder.hpp"
#include "../utils/Cone.hpp"
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

std::vector<Light> lights;

Point3 amb_light(.3, .3, .3);
Point4 observer_pos(0, 0, 0);

Point4 lookFrom(65.0f, 15.0f, 85.0f);
Point4 lookAt(30.f, 0.0f, 50.0f);
Vector4 vUp(0.0f, 1.0f, 0.0f, 0.0f);
Vector4 u, v_cam, w;

std::vector<std::unique_ptr<Object>> world;

// void convertDisplayToWindow(int display_x, int display_y, float &ndc_x, float& ndc_y) {
//   ndc_x = -wWindow/2.0f + dx/2.0f + display_x*dx;
//   ndc_y = hWindow/2.0f - dy/2.0f - display_y*dy;
// }

void convertDisplayToWindow(int display_x, int display_y, float &ndc_x, float& ndc_y) {
    // calculate the size of the window
    float width_w = xmax - xmin;
    float height_w = ymax - ymin;

    // calculate the size of each pixel
    float local_dx = width_w / nCol;
    float local_dy = height_w / nLin;

    // map the pixel (0, 0) to the left upper corner and add local_dx/2 to get the center of the pixel
    ndc_x = xmin + local_dx/2.0f + (display_x * local_dx);
    ndc_y = ymax - local_dy/2.0f - (display_y * local_dy);
}

float hash(Vector4 v) {
    // Produto escalar com valores "mágicos" para espalhar os pontos
    float d = dot(v, Vector4(12.9898, 78.233, 45.164, 9.456));
    return std::fmod(std::sin(d) * 43758.5453f, 1.0f);
}

Point3 getStarryBackground(const Vector4& dir) {
    // 1. Criar um degradê escuro para o céu (Preto -> Azul Marinho)
    float t = 0.5f * (dir.y + 1.0f);
    Point3 skyColor = (1.0f - t) * Point3(0.0, 0.0, 0.05) + t * Point3(0.02, 0.02, 0.1);

    // 2. Gerar as estrelas
    // O hash cria um valor de 0 a 1 para cada direção
    float starIntensity = hash(dir);

    // Filtro para que apenas alguns pontos brilhem muito (densidade das estrelas)
    if (starIntensity > 0.996f) { 
        // Aumenta o brilho da estrela para se destacar do fundo
        float sparkle = std::pow((starIntensity - 0.998f) / (1.0f - 0.998f), 4.0);
        return Point3(1.0, 1.0, 1.0) * sparkle;
    }

    return skyColor;
}

Point3 setColor(const Vector4 &d, HitRecord rec, std::vector<Light> lights){
  Point3 obj_color = rec.obj_ptr->getColor();

  // applying texture
  if(rec.texture != nullptr && !rec.texture->colors.empty()) {
    float u = rec.uv.x;
    float v = rec.uv.y;
    // fixing values that are negative or greater than 1
    u = u - std::floor(u);
    v = v - std::floor(v);

    // inverting v
    v = 1.0f - v;

    // calculating indexes
    int int_u = (int)(u * (rec.texture->width - 1));
    int int_v = (int)(v * (rec.texture->height - 1));

    int_u = std::clamp(int_u, 0, rec.texture->width - 1);
    int_v = std::clamp(int_v, 0, rec.texture->height - 1);

    uint8_t r = std::get<0>(rec.texture->colors[int_v][int_u]);
    uint8_t g = std::get<1>(rec.texture->colors[int_v][int_u]);
    uint8_t b = std::get<2>(rec.texture->colors[int_v][int_u]);
    float r_normalized = r / 255.0f;
    float g_normalized = g / 255.0f;
    float b_normalized = b / 255.0f;
    obj_color.x  = r_normalized;
    obj_color.y = g_normalized;
    obj_color.z = b_normalized;
  }
  Point3 final_color = obj_color * amb_light;

  for (const auto& l : lights) {
    Vector4 light_dir;
    float dist_to_light;
    float intensity = 1.0f;

    if (l.type == LightType::DIRECTIONAL) {
      light_dir = -l.direction; 
      light_dir.normalize();
      dist_to_light = 1e6f; // "infinite"
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

    if (intensity <= 0.0f) continue; // out of the spotlight

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
      // diffuse
      float dif_i = std::max(0.f, dot(rec.normal, light_dir)) * intensity;
      Point3 diff_part = (obj_color * l.color) * dif_i; 
      final_color = final_color + diff_part;

      // specular
      Vector4 reflection = reflect(rec.normal, light_dir);
      float spec_i = std::pow(std::max(0.f, dot(reflection, -d)), 50) * intensity;
      Point3 spec_part = (rec.obj_ptr->getSpecular() * l.color) * spec_i;
      final_color = final_color + spec_part;
    }
  }

  final_color.clamp();
  return final_color;
}

void raycast(std::ofstream &image, int lin_start, int col_start, int width, int height) {
  
  // cavalier: scale = 1.0 (profundidade real)
  // cabinet:  scale = 0.5 (profundidade "mais natural")
  float oblique_scale = 0.5f; 
  float oblique_angle_rad = 0.0f * (3.14159f / 180.0f); // 45 graus

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

      float closest_so_far = 99999;
      HitRecord rec;
      bool hit_anything = false;
      
      for(const auto& object : world){
        HitRecord temp_rec;
        if(object->Intersect(ray_origin, ray_dir, 0, closest_so_far, temp_rec)){
          hit_anything = true;
          closest_so_far = temp_rec.t;
          rec = temp_rec;
        }
      }

      if(hit_anything){
        Point3 final_color = setColor(ray_dir, rec, lights);
        
        int r_int = (int)(final_color.x * 255);
        int g_int = (int)(final_color.y * 255);
        int b_int = (int)(final_color.z * 255);

        image << r_int << " " << g_int << " " << b_int << " ";
      } else {
        Point3 starry_night = getStarryBackground(ray_dir);

        int r_int = (int)(starry_night.x * 255);
        int g_int = (int)(starry_night.y * 255);
        int b_int = (int)(starry_night.z * 255);

        image << r_int << " " << g_int << " " << b_int << " ";
      }
    }
    image << "\n";
  }
}

void fill_xyz(std::string line, float &x, float &y, float &z){
  bool is_negative = false;
  int control = 0; // to control if the next number is for x, y or z (0 = x, 1 = y, 2 = z)
  // start at 2 because we already know it starts with v and something else that isn't a digit
  for(int i = 2; i < line.size(); i++){
    if(line[i] == '-') is_negative = true;
    // if its a digit
    else if(line[i] >= 48 && line[i] <= 57){
      int k = i;
      std::string digit;
      if(is_negative) digit += '-';
      // iterate throughout the digits
      while(line[k] >= 48 && line[k] <= 57 || line[k] == '.'){
        digit += line[k];
        k++;
      }
      // jump i to where k stopped to avoid looping the same number
      i += k-i;
      // reset the negative control var
      is_negative = false;
      // check whether this was x, y or z
      if(control == 0) x = std::stof(digit);
      else if(control == 1) y = std::stof(digit);
      else if(control == 2) z = std::stof(digit);
      //advance the control var
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
      // if its just a vertex
      if(line[1] == ' '){
        float x, y, z;
        fill_xyz(line, x, y, z);
        if(first_vertice){
          min_x = max_x = x;
          min_y = max_y = y;
          min_z = max_z = z;
          first_vertice = false;
        } else {
          if(x < min_x) min_x = x;
          if(x > max_x) max_x = x;
          if(y < min_y) min_y = y;
          if(y > max_y) max_y = y;
          if(z < min_z) min_z = z;
          if(z > max_z) max_z = z;
        }
        v.push_back(std::make_unique<Point4>(x, y, z, 1));
      } else if(line[1] == 'n'){
        // if its a vertex normal
        float x, y, z;
        fill_xyz(line, x, y, z);
        vn.push_back(std::make_unique<Vector4>(x, y, z, 0));
      } else if(line[1] == 't'){
        // if its a texture vertex
        float x, y, z = 0;
        // it supports just x and y, and z wont be used or filled
        fill_xyz(line, x, y, z);
        vt.push_back(std::make_unique<Point3>(x, y, z));
      }
    } else if(line[0] == 'f'){
      // to check if we're reading point 1, 2 or 3 of the triangle
      int point_control = 0;
      // to check if we're reading a vertex, texture vertex or vertex normal
      int vertex_control = 0;
      int vertex_indices[4] = {0, 0, 0, -1};
      int tex_vertex_indices[4] = {0, 0, 0, 0};
      int nor_vertex_indices[4] = {0, 0, 0, 0};
      // skip 2 because it starts with f and blank
      for(int i = 2; i < line.size(); i++){
        if(line[i] == ' ') {
            point_control++;
            vertex_control = 0;
        }
        if(line[i] == '/') vertex_control++;
        // if its a digit
        if(line[i] >= 48 && line[i] <= 57){
          int k = i;
          std::string digit;
          // iterate throughout the digit
          while(line[k] >= 48 && line[k] <= 57){
            digit += line[k];
            k++;
          }
          // check if it was a vertex, texture vertex or vertex normal
          if(vertex_control == 0) vertex_indices[point_control] = std::stoi(digit) - 1;
          else if(vertex_control == 1) tex_vertex_indices[point_control] = std::stoi(digit) - 1;
          else if(vertex_control == 2) nor_vertex_indices[point_control] = std::stoi(digit) - 1;
          i = k - 1;
        }
      }

      // if the faces are not triangulated
      if(vertex_indices[3] != -1){
        Point4 p1 = *v[vertex_indices[0]];
        Point4 p2 = *v[vertex_indices[1]];
        Point4 p3 = *v[vertex_indices[2]];
        Point4 p4 = *v[vertex_indices[3]];
        Vector4 normal = *vn[nor_vertex_indices[0]];

        if(vt.size() > 0){
          Point3 vt1 = *vt[tex_vertex_indices[0]];
          Point3 vt2 = *vt[tex_vertex_indices[1]];
          Point3 vt3 = *vt[tex_vertex_indices[2]];
          Point3 vt4 = *vt[tex_vertex_indices[3]];

          auto new_tri_1 = std::make_unique<Triangle>(p1, p2, p3, normal, vt1, vt2, vt3);
          auto new_tri_2 = std::make_unique<Triangle>(p1, p3, p4, normal, vt1, vt3, vt4);
          Triangle* tri_ptr_1 = new_tri_1.get();
          Triangle* tri_ptr_2 = new_tri_2.get();

          new_tri_1->SetMesh(mesh);
          new_tri_2->SetMesh(mesh);

          f.push_back(std::move(new_tri_1));
          f.push_back(std::move(new_tri_2));
          aabb.t.push_back(tri_ptr_1);
          aabb.t.push_back(tri_ptr_2);
        } else {
          auto new_tri_1 = std::make_unique<Triangle>(p1, p2, p3, normal);
          auto new_tri_2 = std::make_unique<Triangle>(p1, p3, p4, normal);
          Triangle* tri_ptr_1 = new_tri_1.get();
          Triangle* tri_ptr_2 = new_tri_2.get();

          new_tri_1->SetMesh(mesh);
          new_tri_2->SetMesh(mesh);

          f.push_back(std::move(new_tri_1));
          f.push_back(std::move(new_tri_2));
          aabb.t.push_back(tri_ptr_1);
          aabb.t.push_back(tri_ptr_2);
        }
      } else {
        // if the faces are triangulated
        Point4 p1 = *v[vertex_indices[0]];
        Point4 p2 = *v[vertex_indices[1]];
        Point4 p3 = *v[vertex_indices[2]];
        Vector4 normal = *vn[nor_vertex_indices[0]];

        if(vt.size() > 0){
          Point3 vt1 = *vt[tex_vertex_indices[0]];
          Point3 vt2 = *vt[tex_vertex_indices[1]];
          Point3 vt3 = *vt[tex_vertex_indices[2]];

          auto new_tri = std::make_unique<Triangle>(p1, p2, p3, normal, vt1, vt2, vt3);
          Triangle* tri_ptr = new_tri.get();

          new_tri->SetMesh(mesh);

          f.push_back(std::move(new_tri));
          aabb.t.push_back(tri_ptr);
        } else {
          auto new_tri = std::make_unique<Triangle>(p1, p2, p3, normal);
          Triangle* tri_ptr = new_tri.get();

          new_tri->SetMesh(mesh);

          f.push_back(std::move(new_tri));
          aabb.t.push_back(tri_ptr);
        } 
      }
    }
  }

  centroid.x = (max_x + min_x)/2;
  centroid.y = (max_y + min_y)/2;
  centroid.z = (max_z + min_z)/2;

  aabb.min_x = min_x;
  aabb.max_x = max_x;
  aabb.min_y = min_y;
  aabb.max_y = max_y;
  aabb.min_z = min_z;
  aabb.max_z = max_z;

  return;
}

float random_float2() {
  static std::mt19937 generator(
    std::chrono::high_resolution_clock::now().time_since_epoch().count()
  );

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

int main() {
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

  lights.push_back(directional);
  lights.push_back(post_spot);
  lights.push_back(post_spot2);

  // vscode code region
  #pragma region world objects
  auto car1 = createMesh("car_1.obj", "textures/car_1.ppm");  

  car1->applyTranslate(translate(Vector4(-car1->centroid.x, -car1->centroid.y, -car1->centroid.z)));
  car1->applyScale(scale(Vector4(0.1, 0.1, 0.1)));
  Vector4 A_factors(0.5f, 0.0f, 0.0f);
  Vector4 B_factors(0.0f, 0.0f, 0.0f);
  car1->applyShear(shear(A_factors, B_factors));
  car1->applyRotation(rotate(Vector4(0.0f, 1.0f, 0.0f), -90.0f * M_PI / 180.0f));
  float car_half_height = (car1->aabb.max_y - car1->aabb.min_y) / 2.0f;
  car1->applyTranslate(translate(Vector4(20.0f, car_half_height, 30.0f)));


  auto car2 = createMesh("car_1.obj", "textures/car_1.ppm");

  car2->applyTranslate(translate(Vector4(-car2->centroid.x, -car2->centroid.y, -car2->centroid.z)));
  car2->applyScale(scale(Vector4(0.1f, 0.1f, 0.1f)));
  car2->applyRotation(rotate(Vector4(0.0f, 1.0f, 0.0f), -90.0f * M_PI / 180.0f));
  car2->applyTranslate(translate(Vector4(30.0f, car_half_height, 30.0f)));

  auto car3 = createMesh("car_1.obj", "textures/car_1.ppm");

  car3->applyTranslate(translate(Vector4(-car3->centroid.x, -car3->centroid.y, -car3->centroid.z)));
  car3->applyScale(scale(Vector4(0.1f, 0.1f, 0.1f)));
  car3->applyRotation(rotate(Vector4(0.0f, 1.0f, 0.0f), -90.0f * M_PI / 180.0f));
  car3->applyTranslate(translate(Vector4(40.0f, car_half_height, 30.0f)));

  auto cube = createMesh("cube.obj", "");

  cube->applyTranslate(translate(Vector4(-cube->centroid.x, -cube->centroid.y, -cube->centroid.z)));
  cube->applyScale(scale(Vector4(25.0f, 8.0f, 10.0f)));
  float half_cube_height = (cube->aabb.max_y - cube->aabb.min_y) / 2.0f;
  cube->applyTranslate(translate(Vector4(0.0f, half_cube_height, -25.0f)));

  for(auto& face : cube->faces){
    face->reflectivity = 1.0f;
  }

  auto shop = createMesh("loja.obj", "textures/loja.ppm");

  shop->applyTranslate(translate(Vector4(-shop->centroid.x, -shop->centroid.y, -shop->centroid.z)));
  shop->applyScale(scale(Vector4(6.0f, 10.0f, 5.0f)));
  shop->applyRotation(rotate(Vector4(1.0f, 0.0f, 0.0f), 90.0f * M_PI / 180.0f));
  float half_shop_height = (shop->aabb.max_y - shop->aabb.min_y) / 2.0f;
  shop->applyTranslate(translate(Vector4(30.0f, half_shop_height, 5.0f)));

  auto road = createMesh("cube.obj", "");

  road->applyTranslate(translate(Vector4(-road->centroid.x, -road->centroid.y, -road->centroid.z)));
  road->applyScale(scale(Vector4(60.0f, 0.01f, 10.0f)));
  float half_road_height = (road->aabb.max_y - road->aabb.min_y) / 2.0f;
  road->applyTranslate(translate(Vector4(30.0f, half_road_height, 50.0f)));

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
  world.push_back(std::move(shop));
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

  // vscode code region
  #pragma region plains
  Point3 specular_plains(.1, .1, .1);
  Point3 back_wall_col(.9, .3, .5);
  Point3 front_wall_col(.5, .7, 1);
  Point3 left_wall_col(.1, .5, .5);
  Point3 right_wall_col(.6, .2, .7);
  Point3 ceiling_col(.2, .2, .9);
  Point3 floor_col(.9, .5, 0);
  
  // // back wall
  // world.push_back(std::make_unique<Plain>(Point4(0, 0, -200), Vector4(0, 0, 1), back_wall_col, back_wall_col, specular_plains));
  // // front wall
  // world.push_back(std::make_unique<Plain>(Point4(0, 0, 100), Vector4(0, 0, -1), front_wall_col, front_wall_col, specular_plains));
  // // left wall
  // world.push_back(std::make_unique<Plain>(Point4(-100, 0, 0), Vector4(1, 0, 0), left_wall_col, left_wall_col, specular_plains));
  // // right wall
  // world.push_back(std::make_unique<Plain>(Point4(100, 0, 0), Vector4(-1, 0, 0), right_wall_col, right_wall_col, specular_plains));
  // // ceiling
  // world.push_back(std::make_unique<Plain>(Point4(0, 100, 0), Vector4(0, -1, 0), ceiling_col, ceiling_col, specular_plains));
  // floor
  world.push_back(std::make_unique<Plain>(Point4(0, 0, 0), Vector4(0, 1, 0), floor_col, floor_col, specular_plains));
  #pragma endregion

  w = (lookFrom - lookAt); 
  w.normalize();
  u = cross(vUp, w); 
  u.normalize();
  v_cam = cross(w, u);

  auto full_start = std::chrono::high_resolution_clock::now();
  int frames = 1;
  
  for(int i = 0; i < frames; i++){
      auto start = std::chrono::high_resolution_clock::now();

      std::string image_name = "frames/frame_";
      if(i < 10) image_name += "00";
      else if(i < 100) image_name += "0";
      image_name += std::to_string(i) + ".ppm";
      
      std::ofstream image(image_name);

      if(image.is_open()) {
          image << "P3\n" << nCol << " " << nLin << "\n255\n";
          raycast(image, 0, 0, nCol, nLin);
          image.close();
      }

      auto stop = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed = stop - start;
      std::cout << "Frame " << i+1 << " rendered in " << elapsed.count() << " seconds.\n";
  }

  auto full_stop = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed_total = full_stop - full_start;
  std::cout << "Total time: " << elapsed_total.count() << " seconds.\n";

  return 0;
}
