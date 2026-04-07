#include "aux_functions.hpp"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <memory>
#include <iostream>
#include <random>
#include <chrono>

float hash(Vector4 v) {
    float d = dot(v, Vector4(12.9898, 78.233, 45.164, 9.456));
    return std::fmod(std::sin(d) * 43758.5453f, 1.0f);
}


float random_float2() {
  static std::mt19937 generator(std::chrono::high_resolution_clock::now().time_since_epoch().count());
  static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
  return distribution(generator);
}

Vector4 reflect_ray(const Vector4& v, const Vector4& n) {
  Vector4 vr;
    vr = v - n * 2.0f * dot(v, n);
    vr.x += random_float2() / 20;
    return vr;
}
Vector4 noise_reflect_ray(const Vector4& v, const Vector4& n) {
  Vector4 vr;
    vr = v - n * 2.0f * dot(v, n);
    vr.x += random_float2() / 20;
    return vr;
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

void convertDisplayToWindow(int display_x, int display_y, float &ndc_x, float& ndc_y, float xmin, float xmax, float ymin, float ymax, int nCol, int nLin) {
    float width_w = xmax - xmin;
    float height_w = ymax - ymin;
    float local_dx = width_w / nCol;
    float local_dy = height_w / nLin;
    ndc_x = xmin + local_dx/2.0f + (display_x * local_dx);
    ndc_y = ymax - local_dy/2.0f - (display_y * local_dy);
}

Point3 setColor(const Vector4 &d, HitRecord rec, std::vector<Light> lights, Point3 amb_light, std::vector<std::unique_ptr<Object>> &world){
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
      Vector4 reflection;
      if(rec.reflectivity == 1)
      {
         noise_reflect_ray(rec.normal, light_dir);
      }
      else{

        reflect_ray(rec.normal, light_dir);
      }
      float spec_i = std::pow(std::max(0.f, dot(reflection, -d)), 50) * intensity;
      Point3 spec_part = (rec.obj_ptr->getSpecular() * l.color) * spec_i;
      final_color = final_color + spec_part;
    }
  }

  final_color.clamp();
  return final_color;
}

float get_t_distance(std::vector<std::unique_ptr<Object>> &world, const Point4& origin, const Vector4& dir){
  float t_distance = 99999.0f;
  bool hit_anything = false;

  for(const auto& object : world){
    HitRecord temp_rec;
    // t_min slightly greater than 0 to avoid acne
    if(object->Intersect(origin, dir, 0.001f, t_distance, temp_rec)){
      hit_anything = true;
      t_distance = temp_rec.t;
    }
  }

  if(hit_anything) return t_distance;

  return 99999.0f;
}

Point3 cast_ray(const Point4& ray_origin, const Vector4& ray_dir, int depth, 
                std::vector<std::unique_ptr<Object>> &world, 
                std::vector<Light> &lights,
                Point3& amb_light) {
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
        Point3 local_color = setColor(ray_dir, rec, lights, amb_light, world);

        float reflectivity = rec.obj_ptr->getReflectivity();

        if (reflectivity > 0.0f) {
            Vector4 reflected_dir = noise_reflect_ray(ray_dir, rec.normal);
            reflected_dir.normalize();

            Point3 reflected_color = cast_ray(rec.p_int, reflected_dir, depth - 1, world, lights, amb_light);

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

void raycast(std::ofstream &image, int lin_start, int col_start, int width, int height, float xmin, float xmax, float ymin, float ymax, int nCol, int nLin, 
              Projection projectionType, Point4 lookFrom, Vector4 u, Vector4 v_cam, Vector4 w, float dWindow, std::vector<std::unique_ptr<Object>> &world, 
              std::vector<Light> lights, std::vector<float> pixels, Point3 amb_light, const Vector4& ray_dir_up, const Vector4& ray_dir_down,
              const Vector4& ray_dir_right, const Vector4& ray_dir_left, bool border_detection, int x, int y) {
  float oblique_scale = 0.5f; 
  float oblique_angle_rad = 0.0f * (3.14159f / 180.0f);

  for(int l = lin_start; l < height; l++){
    for(int c = col_start; c < width; c++){
      float x, y;
      convertDisplayToWindow(c, l, x, y, xmin, xmax, ymin, ymax, nCol, nLin);

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

      Point3 final_color = cast_ray(ray_origin, ray_dir, 3, world, lights, amb_light);

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
    if(line.empty()) continue;

    if(line[0] == 'v'){
      // --- VÉRTICE (v x y z) ---
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
      } 
      // --- NORMAL (vn x y z) ---
      else if(line[1] == 'n'){
        float x, y, z;
        fill_xyz(line, x, y, z);
        vn.push_back(std::make_unique<Vector4>(x, y, z, 0));
      } 
      // --- TEXTURA (vt u v) ---
      else if(line[1] == 't'){
        float x, y, z = 0;
        fill_xyz(line, x, y, z);
        vt.push_back(std::make_unique<Point3>(x, y, z));
      }
    } 
    // --- FACE (f v/vt/vn ...) ---
    else if(line[0] == 'f'){
      int point_control = 0;
      int vertex_control = 0;
      int vertex_indices[4] = {0, 0, 0, -1};
      int tex_vertex_indices[4] = {0, 0, 0, 0};
      int nor_vertex_indices[4] = {0, 0, 0, 0};
      
      // Parser manual da linha da face
      for(size_t i = 2; i < line.size(); i++){
        if(line[i] == ' ') {
            point_control++;
            vertex_control = 0;
            if(point_control > 3) break; // Proteção: ignora vértices extras em polígonos > 4 lados
        }
        else if(line[i] == '/') vertex_control++;
        else if(line[i] >= 48 && line[i] <= 57){
          int k = i;
          std::string digit;
          while(k < line.size() && line[k] >= 48 && line[k] <= 57){
            digit += line[k];
            k++;
          }
          int val = std::stoi(digit) - 1; // Converte para índice base 0
          
          if(vertex_control == 0) vertex_indices[point_control] = val;
          else if(vertex_control == 1) tex_vertex_indices[point_control] = val;
          else if(vertex_control == 2) nor_vertex_indices[point_control] = val;
          i = k - 1;
        }
      }

      // --- CHECAGEM DE SEGURANÇA (SEGFAULT FIX) ---
      auto check_idx = [&](int idx, size_t size) -> bool {
          if(idx < 0 || idx >= (int)size) return false;
          return true;
      };

      // Se os vértices principais não existem, pula a face
      if (!check_idx(vertex_indices[0], v.size()) ||
          !check_idx(vertex_indices[1], v.size()) ||
          !check_idx(vertex_indices[2], v.size())) continue;

      // Verifica se existem normais e texturas válidas
      bool has_normals = !vn.empty() && check_idx(nor_vertex_indices[0], vn.size());
      
      // Só habilita textura se ELA EXISTIR e se os índices forem válidos
      bool has_texture = !vt.empty() && 
                         check_idx(tex_vertex_indices[0], vt.size()) &&
                         check_idx(tex_vertex_indices[1], vt.size()) &&
                         check_idx(tex_vertex_indices[2], vt.size());

      // Verifica se é um quadrado (Quad) e se o 4º vértice é válido
      bool is_quad = (vertex_indices[3] != -1) && check_idx(vertex_indices[3], v.size());

      // Helper para criar e adicionar triângulo
      // Helper para criar e adicionar triângulo
      auto add_triangle = [&](int i0, int i1, int i2, int t0, int t1, int t2) {
          
          // 1. AQUI ESTÁ A MÁGICA: Usamos .get() para pegar o ponteiro cru
          // em vez de usar '*' para copiar o Point4 inteiro.
          Point4* p1 = v[vertex_indices[i0]].get();
          Point4* p2 = v[vertex_indices[i1]].get();
          Point4* p3 = v[vertex_indices[i2]].get();

          // 2. Normais continuam por valor (cópia), usando o '*'
          Vector4 normal = has_normals ? *vn[nor_vertex_indices[i0]] : Vector4(0, 1, 0, 0);

          std::unique_ptr<Triangle> new_tri;

          // Cria triângulo com textura APENAS se tudo estiver válido
          if(has_texture && check_idx(tex_vertex_indices[t0], vt.size()) && 
                            check_idx(tex_vertex_indices[t1], vt.size()) && 
                            check_idx(tex_vertex_indices[t2], vt.size())) {
              
              // 3. Texturas também continuam por valor (cópia)
              Point3 vt1 = *vt[tex_vertex_indices[t0]];
              Point3 vt2 = *vt[tex_vertex_indices[t1]];
              Point3 vt3 = *vt[tex_vertex_indices[t2]];
              
              // 4. Passamos os ponteiros de posição e os valores de normal/textura
              new_tri = std::make_unique<Triangle>(p1, p2, p3, normal, vt1, vt2, vt3);
          } else {
              // Senão, cria sem textura (evita o crash)
              new_tri = std::make_unique<Triangle>(p1, p2, p3, normal);
          }

          // O resto da função continua igualzinho!
          mesh->indices.push_back(vertex_indices[i0]);
          mesh->indices.push_back(vertex_indices[i1]);
          mesh->indices.push_back(vertex_indices[i2]);

          new_tri->SetMesh(mesh);
          Triangle* ptr = new_tri.get();
          f.push_back(std::move(new_tri));
          aabb.t.push_back(ptr);
      };

      // 1º Triângulo da face
      add_triangle(0, 1, 2, 0, 1, 2);

      // 2º Triângulo (se for quadrado)
      if(is_quad){
          add_triangle(0, 2, 3, 0, 2, 3);
      }
    }
  }

  // Finaliza calculando o centróide e AABB
  if (!first_vertice) {
      centroid.x = (max_x + min_x)/2;
      centroid.y = (max_y + min_y)/2;
      centroid.z = (max_z + min_z)/2;

      aabb.min_x = min_x; aabb.max_x = max_x;
      aabb.min_y = min_y; aabb.max_y = max_y;
      aabb.min_z = min_z; aabb.max_z = max_z;
  }
}