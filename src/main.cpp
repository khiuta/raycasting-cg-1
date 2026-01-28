#include "../utils/Point4.hpp"
#include "../utils/Point3.hpp"
#include "../utils/Vector4.hpp"
#include "../utils/Triangle.hpp"
#include "../utils/Object.hpp"
#include "../utils/HitRecord.hpp"
#include "../utils/ListMesh.hpp"
#include "../utils/Plain.hpp"
#include "../utils/AABB.hpp"

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

#define Vector3   rVector3
#define Vector4   rVector4
#define Matrix    rMatrix
#define Texture   rTexture
#define Color     rColor
#define Image     rImage
#define Rectangle rRectangle
#define Camera    rCamera

#include "raylib.h"

#undef Vector3
#undef Vector4
#undef Matrix
#undef Texture
#undef Color
#undef Image
#undef Rectangle
#undef Camera

const rColor rBLACK    = (rColor){ 0, 0, 0, 255 };
const rColor rWHITE    = (rColor){ 255, 255, 255, 255 };
const rColor rRAYWHITE = (rColor){ 245, 245, 245, 255 };
const rColor rGREEN    = (rColor){ 0, 228, 48, 255 };
const rColor rYELLOW   = (rColor){ 253, 249, 0, 255 };

// =========================================================

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Ajustando para usar a resolução da janela
const float wWindow = 4.f, hWindow = 3.f;
const int nCol = WINDOW_WIDTH;
const int nLin = WINDOW_HEIGHT;

float dx = wWindow / nCol;
float dy = hWindow / nLin;
float dWindow = 4.0f;

enum class Projection{
  Perspective,
  Ortographic,
  Oblique
};
Projection projectionType = Projection::Perspective;

Point4 lightPos(0.0f, 10.0f, 40.0f);
Point3 amb_light(.3, .3, .3);
Point4 observer_pos(0, 0, 0);

Point4 lookFrom(0.0f, 5.0f, 15.0f);
Point4 lookAt(0.0f, 0.0f, 0.0f);
Vector4 vUp(0.0f, 1.0f, 0.0f, 0.0f); // Usa sua classe Vector4
Vector4 u, v_cam, w;

std::vector<std::unique_ptr<Object>> world;

void convertDisplayToWindow(int display_x, int display_y, float &ndc_x, float& ndc_y) {
  ndc_x = -wWindow/2.0f + dx/2.0f + display_x*dx;
  ndc_y = hWindow/2.0f - dy/2.0f - display_y*dy;
}

Point3 setColor(const Vector4 &d, HitRecord rec, const Point4 &light_pos){
  Point3 obj_color = rec.obj_ptr->getColor();
  Point3 mat_dif = rec.obj_ptr->getDiffuse();

  // Texturização
  if(rec.texture != nullptr) {
    float u_tex = rec.uv.x;
    float v_tex = rec.uv.y;

    u_tex = u_tex - std::floor(u_tex);
    v_tex = v_tex - std::floor(v_tex);
    v_tex = 1.0f - v_tex;

    int int_u = (int)(u_tex * (rec.texture->width - 1));
    int int_v = (int)(v_tex * (rec.texture->height - 1));

    int_u = std::clamp(int_u, 0, rec.texture->width - 1);
    int_v = std::clamp(int_v, 0, rec.texture->height - 1);

    uint8_t r = std::get<0>(rec.texture->colors[int_v][int_u]);
    uint8_t g = std::get<1>(rec.texture->colors[int_v][int_u]);
    uint8_t b = std::get<2>(rec.texture->colors[int_v][int_u]);
    
    obj_color.x = mat_dif.x = r / 255.0f;
    obj_color.y = mat_dif.y = g / 255.0f;
    obj_color.z = mat_dif.z = b / 255.0f;
  }

  Point3 mat_spec = rec.obj_ptr->getSpecular();
  Point3 amb_color(obj_color.x*amb_light.x, obj_color.y*amb_light.y, obj_color.z*amb_light.z);

  Vector4 light_dir = light_pos - rec.p_int;
  float dist_to_light = light_dir.length();
  light_dir.normalize();

  
  Vector4 d_inv = -d; 
  d_inv.normalize();

  bool on_shadow = false;
  for(const auto& other : world) {
    if(other.get() == rec.obj_ptr) continue;
    HitRecord temp_rec;
    if(other->Intersect(rec.p_int, light_dir, 0.001f, dist_to_light, temp_rec)){
      on_shadow = true;
      break;
    }
  }
  if(on_shadow) return amb_color;

  Point3 diff_light(1, 1, 1);
  float dif_i = std::max(0.f, dot(rec.normal, light_dir));
  Point3 diff_color(
    diff_light.x*mat_dif.x*dif_i,
    diff_light.y*mat_dif.y*dif_i,
    diff_light.z*mat_dif.z*dif_i
  );

  Point3 spec_light(1, 1, 1);
  int brightness = 50; 
  Vector4 reflection = reflect(rec.normal, light_dir);
  float spec_i = pow(std::max(0.f, dot(reflection, d_inv)), brightness);
  Point3 spec_color(
    spec_light.x*mat_spec.x*spec_i,
    spec_light.y*mat_spec.y*spec_i,
    spec_light.z*mat_spec.z*spec_i
  );

  Point3 final_color(amb_color+diff_color+spec_color);
  final_color.clamp();
  return final_color;
}

// Recebe 'rImage' (struct Image da Raylib renomeada)
void raycast(rImage &buffer, int width, int height) {
  
  float oblique_scale = 0.5f; 
  float oblique_angle_rad = 45.0f * (3.14159f / 180.0f); 

  // Loop de renderização
  for(int l = 0; l < height; l++){
    for(int c = 0; c < width; c++){
      float x, y;
      convertDisplayToWindow(c, l, x, y);

      Point4 ray_origin;
      Vector4 ray_dir;

      if (projectionType == Projection::Perspective) {
          // Origem fixa, direção varia
          ray_origin = lookFrom;
          ray_dir = (u * x) + (v_cam * y) - (w * dWindow);
          ray_dir.normalize();
      } 
      else if (projectionType == Projection::Ortographic) {
          // Origem varia, direção fixa (frente)
          ray_origin = lookFrom + (u * x) + (v_cam * y);
          ray_dir = -w; 
          ray_dir.normalize();
      }
      else if (projectionType == Projection::Oblique) {
          // Origem varia, direção fixa (inclinada)
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
        // Intersect usa a sua classe Vector4/Point4 (correto)
        if(object->Intersect(ray_origin, ray_dir, 0, closest_so_far, temp_rec)){
          hit_anything = true;
          closest_so_far = temp_rec.t;
          rec = temp_rec;
        }
      }

      // Usa 'rColor' (Color da Raylib renomeada) para pintar
      rColor pixelColor;

      if(hit_anything){
        Point3 final_color = setColor(ray_dir, rec, lightPos);
        pixelColor = (rColor){
            (unsigned char)(final_color.x * 255),
            (unsigned char)(final_color.y * 255),
            (unsigned char)(final_color.z * 255),
            255
        };
      } else {
        pixelColor = (rColor){100, 100, 100, 255};
      }

      ImageDrawPixel(&buffer, c, l, pixelColor);
    }
  }
}

// Funções de leitura de arquivo OBJ
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
        if(first_vertice){
          min_x = max_x = x; min_y = max_y = y; min_z = max_z = z; first_vertice = false;
        } else {
          if(x < min_x) min_x = x; if(x > max_x) max_x = x;
          if(y < min_y) min_y = y; if(y > max_y) max_y = y;
          if(z < min_z) min_z = z; if(z > max_z) max_z = z;
        }
        v.push_back(std::make_unique<Point4>(x, y, z, 1));
      } else if(line[1] == 'n'){
        float x, y, z;
        fill_xyz(line, x, y, z);
        vn.push_back(std::make_unique<Vector4>(x, y, z, 0));
      } else if(line[1] == 't'){
        float x, y, z = 0;
        fill_xyz(line, x, y, z);
        vt.push_back(std::make_unique<Point3>(x, y, z));
      }
    } else if(line[0] == 'f'){
      int point_control = 0;
      int vertex_control = 0;
      int vertex_indices[4] = {0, 0, 0, -1};
      int tex_vertex_indices[4] = {0, 0, 0, 0};
      int nor_vertex_indices[4] = {0, 0, 0, 0};
      
      for(int i = 2; i < line.size(); i++){
        if(line[i] == ' ') { point_control++; vertex_control = 0; }
        if(line[i] == '/') vertex_control++;
        if(line[i] >= 48 && line[i] <= 57){
          int k = i;
          std::string digit;
          while(line[k] >= 48 && line[k] <= 57){ digit += line[k]; k++; }
          if(vertex_control == 0) vertex_indices[point_control] = std::stoi(digit) - 1;
          else if(vertex_control == 1) tex_vertex_indices[point_control] = std::stoi(digit) - 1;
          else if(vertex_control == 2) nor_vertex_indices[point_control] = std::stoi(digit) - 1;
          i = k - 1;
        }
      }

      if(vertex_indices[3] != -1){
        Point4 p1 = *v[vertex_indices[0]]; Point4 p2 = *v[vertex_indices[1]];
        Point4 p3 = *v[vertex_indices[2]]; Point4 p4 = *v[vertex_indices[3]];
        Vector4 normal = *vn[nor_vertex_indices[0]]; // Usa sua classe Vector4

        if(vt.size() > 0){
          Point3 vt1 = *vt[tex_vertex_indices[0]]; Point3 vt2 = *vt[tex_vertex_indices[1]];
          Point3 vt3 = *vt[tex_vertex_indices[2]]; Point3 vt4 = *vt[tex_vertex_indices[3]];
          
          auto t1 = std::make_unique<Triangle>(p1, p2, p3, normal, vt1, vt2, vt3);
          auto t2 = std::make_unique<Triangle>(p1, p3, p4, normal, vt1, vt3, vt4);
          t1->SetMesh(mesh); t2->SetMesh(mesh);
          f.push_back(std::move(t1)); f.push_back(std::move(t2));
          aabb.t.push_back(f[f.size()-2].get()); aabb.t.push_back(f.back().get());
        } else {
          auto t1 = std::make_unique<Triangle>(p1, p2, p3, normal);
          auto t2 = std::make_unique<Triangle>(p1, p3, p4, normal);
          t1->SetMesh(mesh); t2->SetMesh(mesh);
          f.push_back(std::move(t1)); f.push_back(std::move(t2));
          aabb.t.push_back(f[f.size()-2].get()); aabb.t.push_back(f.back().get());
        }
      } else {
        Point4 p1 = *v[vertex_indices[0]]; Point4 p2 = *v[vertex_indices[1]];
        Point4 p3 = *v[vertex_indices[2]]; Vector4 normal = *vn[nor_vertex_indices[0]];
        if(vt.size() > 0){
          Point3 vt1 = *vt[tex_vertex_indices[0]]; Point3 vt2 = *vt[tex_vertex_indices[1]]; Point3 vt3 = *vt[tex_vertex_indices[2]];
          auto t = std::make_unique<Triangle>(p1, p2, p3, normal, vt1, vt2, vt3);
          t->SetMesh(mesh); f.push_back(std::move(t)); aabb.t.push_back(f.back().get());
        } else {
          auto t = std::make_unique<Triangle>(p1, p2, p3, normal);
          t->SetMesh(mesh); f.push_back(std::move(t)); aabb.t.push_back(f.back().get());
        } 
      }
    }
  }

  centroid.x = (max_x + min_x)/2; centroid.y = (max_y + min_y)/2; centroid.z = (max_z + min_z)/2;
  aabb.min_x = min_x; aabb.max_x = max_x; aabb.min_y = min_y; aabb.max_y = max_y; aabb.min_z = min_z; aabb.max_z = max_z;
}

int main() {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Raycasting CG1");
  SetTargetFPS(60);

  // Cria buffer na CPU (usando rBLACK em vez de BLACK)
  rImage screenBuffer = GenImageColor(WINDOW_WIDTH, WINDOW_HEIGHT, rBLACK);
  // Carrega textura na GPU
  Texture2D screenTexture = LoadTextureFromImage(screenBuffer);

  // --- CARREGAMENTO DO MUNDO ---
  std::string obj_name = "car_1.obj";

  std::vector<std::unique_ptr<Point4>> v;
  std::vector<std::unique_ptr<Vector4>> vn;
  std::vector<std::unique_ptr<Point3>> vt;
  std::vector<std::unique_ptr<Triangle>> f;
  Point4 centroid;
  AABB aabb;
  
  std::unique_ptr<ListMesh> cube = std::make_unique<ListMesh>("textures/car_1.ppm");
  
  read_obj_file(obj_name, v, vn, vt, f, centroid, aabb, cube.get());
  cube->aabb = std::move(aabb);
  cube->faces = std::move(f);
  cube->vertices = std::move(v);
  cube->centroid = std::move(centroid);
  std::cout << "Loaded " << cube->faces.size() << " triangles on object " << obj_name << "\n";
  
  cube->applyTranslate(translate(Vector4(-cube->centroid.x, -cube->centroid.y, -cube->centroid.z)));
  cube->applyScale(scale(Vector4(0.1, 0.1, 0.1)));
  cube->aabb.buildBVH(10);
  world.push_back(std::move(cube));

  #pragma region plains
  Point3 specular_plains(.1, .1, .1);
  Point3 back_wall_col(.9, .3, .5);
  Point3 front_wall_col(.5, .7, 1);
  Point3 left_wall_col(.1, .5, .5);
  Point3 right_wall_col(.6, .2, .7);
  Point3 ceiling_col(.2, .2, .9);
  Point3 floor_col(.9, .5, 0);
  
  world.push_back(std::make_unique<Plain>(Point4(0, 0, -200), Vector4(0, 0, 1), back_wall_col, back_wall_col, specular_plains));
  world.push_back(std::make_unique<Plain>(Point4(0, 0, 100), Vector4(0, 0, -1), front_wall_col, front_wall_col, specular_plains));
  world.push_back(std::make_unique<Plain>(Point4(-100, 0, 0), Vector4(1, 0, 0), left_wall_col, left_wall_col, specular_plains));
  world.push_back(std::make_unique<Plain>(Point4(100, 0, 0), Vector4(-1, 0, 0), right_wall_col, right_wall_col, specular_plains));
  world.push_back(std::make_unique<Plain>(Point4(0, 100, 0), Vector4(0, -1, 0), ceiling_col, ceiling_col, specular_plains));
  world.push_back(std::make_unique<Plain>(Point4(0, -50, 0), Vector4(0, 1, 0), floor_col, floor_col, specular_plains));
  #pragma endregion

  float raio = 20.0f;
  float altura_camera = 5.0f;
  float angle = 0.0f;
  bool paused = false;

  // --- LOOP PRINCIPAL ---
  while (!WindowShouldClose()) {
    
    // Controles
    if (IsKeyPressed(KEY_ONE)) projectionType = Projection::Perspective;
    if (IsKeyPressed(KEY_TWO)) projectionType = Projection::Ortographic;
    if (IsKeyPressed(KEY_THREE)) projectionType = Projection::Oblique;
    if (IsKeyPressed(KEY_SPACE)) paused = !paused;

    // Rotação
    if(!paused) angle += 0.02f;

    float novo_x = lookAt.x + raio * std::sin(angle);
    float novo_z = lookAt.z + raio * std::cos(angle);
    lookFrom = Point4(novo_x, altura_camera, novo_z);

    w = (lookFrom - lookAt); w.normalize();
    u = cross(vUp, w); u.normalize();
    v_cam = cross(w, u);

    // 1. Raytracing na CPU
    raycast(screenBuffer, WINDOW_WIDTH, WINDOW_HEIGHT);

    // 2. Upload para GPU
    UpdateTexture(screenTexture, screenBuffer.data);

    // 3. Desenho na Tela
    BeginDrawing();
      // Usando rRAYWHITE e rWHITE
      ClearBackground(rRAYWHITE);
      DrawTexture(screenTexture, 0, 0, rWHITE);

      DrawFPS(10, 10);
      DrawText("1: Persp | 2: Ortho | 3: Oblique", 10, 30, 20, rGREEN);
      DrawText(TextFormat("Projection: %d", (int)projectionType), 10, 50, 20, rYELLOW);
      
    EndDrawing();
  }

  // Limpeza
  UnloadTexture(screenTexture);
  UnloadImage(screenBuffer);
  CloseWindow();

  return 0;
}