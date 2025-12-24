#include "../utils/Point4.hpp"
#include "../utils/Point3.hpp"
#include "../utils/Vector4.hpp"
#include "../utils/Triangle.hpp"
#include "../utils/Object.hpp"
#include "../utils/HitRecord.hpp"
#include "../utils/ListMesh.hpp"
#include "../utils/Matrix4.hpp"
#include "../utils/Plain.hpp"
#include "../utils/Sphere.hpp"
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

const float wWindow = 4.f, hWindow = 3.f;
const int nCol = wWindow*200, nLin = hWindow*200;
float dx = wWindow / nCol;
float dy = hWindow / nLin;
float dWindow = 4.0f;

Point4 lightPos(20, 10, 0);
Point3 amb_light(.3, .3, .3);
Point4 observer_pos(0, 0, 0);

Point4 lookFrom(0.0f, 5.0f, 15.0f);
Point4 lookAt(0.0f, 0.0f, 0.0f);
Vector4 vUp(0.0f, 1.0f, 0.0f, 0.0f);

Vector4 u, v_cam, w;

std::vector<std::unique_ptr<Object>> world;

void convertDisplayToWindow(int display_x, int display_y, float &ndc_x, float& ndc_y) {
  ndc_x = -wWindow/2.0f + dx/2.0f + display_x*dx;
  ndc_y = hWindow/2.0f - dy/2.0f - display_y*dy;
}

Point3 setColor(const Vector4 &d, HitRecord rec, const Point4 &light_pos){
  if(rec.texture.size() > 0) {
    int int_u = (int)std::round(rec.uv.x);
    int int_v = (int)std::round(rec.uv.y);

    Point3 tex_color();
  }
  Point3 obj_color = rec.obj_ptr->getColor();
  Point3 mat_dif = rec.obj_ptr->getDiffuse();
  Point3 mat_spec = rec.obj_ptr->getSpecular();

  Point3 amb_color(obj_color.x*amb_light.x, obj_color.y*amb_light.y, obj_color.z*amb_light.z);
  
  Vector4 light_dir = light_pos - rec.p_int;
  float dist_to_light = light_dir.length();
  light_dir.normalize();
  Vector4 d_inv = lookFrom - rec.p_int;
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

  // diffuse lighting
  Point3 diff_light(1, 1, 1);
  float dif_i = std::max(0.f, dot(rec.normal, light_dir));
  Point3 diff_color(
    diff_light.x*mat_dif.x*dif_i,
    diff_light.y*mat_dif.y*dif_i,
    diff_light.z*mat_dif.z*dif_i
  );

  // specular lighting
  Point3 spec_light(1, 1, 1);
  int brightness = 50; // light scattering factor
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

void raycast(std::ofstream &image, int lin_start, int col_start, int width, int height) {
  for(int l = lin_start; l < lin_start+height; l++){
    for(int c = col_start; c < col_start+width; c++){
      float x, y;
      convertDisplayToWindow(c, l, x, y);

      //Vector4 d(x - observer_pos.x, y - observer_pos.y, -dWindow);
      Vector4 d = (u * x) + (v_cam * y) - (w * dWindow);
      d.normalize();

      float closest_so_far = 99999;
      HitRecord rec;
      bool hit_anything = false;
      for(const auto& object : world){
        HitRecord temp_rec;
        if(object->Intersect(lookFrom, d, 0, closest_so_far, temp_rec)){
          hit_anything = true;
          closest_so_far = temp_rec.t;
          rec = temp_rec;
        }
      }

      if(hit_anything){
        Point3 final_color = setColor(d, rec, lightPos);
        int r_int = (int)(final_color.x * 255);
        int g_int = (int)(final_color.y * 255);
        int b_int = (int)(final_color.z * 255);
        
        image << r_int << " " << g_int << " " << b_int << " ";
      } else {
        image << 100 << " " << 100 << " " << 100 << " ";
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
        Point4 p1 = *v[vertex_indices[0]];
        Point4 p2 = *v[vertex_indices[1]];
        Point4 p3 = *v[vertex_indices[2]];
        Vector4 normal = *vn[nor_vertex_indices[0]];

        auto new_tri = std::make_unique<Triangle>(p1, p2, p3, normal);
        Triangle* tri_ptr = new_tri.get();
        f.push_back(std::move(new_tri));
        aabb.t.push_back(tri_ptr);
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

int main() {
  std::string obj_name = "cube_nt.obj";

  std::vector<std::unique_ptr<Point4>> v;
  std::vector<std::unique_ptr<Vector4>> vn;
  std::vector<std::unique_ptr<Point3>> vt;
  std::vector<std::unique_ptr<Triangle>> f;
  Point4 centroid;
  AABB aabb;
  std::unique_ptr<ListMesh> cube = std::make_unique<ListMesh>("textures/obamium.ppm");
  read_obj_file(obj_name, v, vn, vt, f, centroid, aabb, cube.get());
  std::cout << "Loaded " << f.size() << " triangles on object " << obj_name << "\n";

  //std::unique_ptr<ListMesh> cube = std::make_unique<ListMesh>(std::move(f), std::move(v), centroid, std::move(aabb), "textures/obamium.ppm");
  cube->aabb.buildBVH(10);
  cube->texture.loadTexture();
  world.push_back(std::move(cube));

  #pragma region plains
  Point3 specular_plains(.1, .1, .1);
  Point3 back_wall_col(.9, .3, .5);
  Point3 front_wall_col(.5, .7, 1);
  Point3 left_wall_col(.1, .5, .5);
  Point3 right_wall_col(.6, .2, .7);
  Point3 ceiling_col(.2, .2, .9);
  Point3 floor_col(.9, .5, 0);
  Plain back_wall(Point4(0, 0, -200), Vector4(0, 0, 1), back_wall_col, back_wall_col, specular_plains);
  Plain front_wall(Point4(0, 0, 100), Vector4(0, 0, -1), front_wall_col, front_wall_col, specular_plains);
  Plain left_wall(Point4(-100, 0, 0), Vector4(1, 0, 0), left_wall_col, left_wall_col, specular_plains);
  Plain right_wall(Point4(100, 0, 0), Vector4(-1, 0, 0), right_wall_col, right_wall_col, specular_plains);
  Plain ceiling(Point4(0, 100, 0), Vector4(0, -1, 0), ceiling_col, ceiling_col, specular_plains);
  Plain floor(Point4(0, -50, 0), Vector4(0, 1, 0), floor_col, floor_col, specular_plains);

  world.push_back(std::make_unique<Plain>(back_wall));
  world.push_back(std::make_unique<Plain>(front_wall));
  world.push_back(std::make_unique<Plain>(left_wall));
  world.push_back(std::make_unique<Plain>(right_wall));
  world.push_back(std::make_unique<Plain>(ceiling));
  world.push_back(std::make_unique<Plain>(floor));
  #pragma endregion

  w = (lookFrom - lookAt); // Vetor apontando para trás
  w.normalize();

  u = cross(vUp, w); // Vetor apontando para a direita
  u.normalize();

  v_cam = cross(w, u);

  float raio = 20.0f; // Distância da câmera ao cubo
  float altura_camera = 5.0f;

  int frames = 180;
  float x = random_float2();
  float y = random_float2();
  float z = random_float2();
  auto full_start = std::chrono::high_resolution_clock::now();
  for(int i = 0; i < frames; i++){
    auto start = std::chrono::high_resolution_clock::now();

    std::string image_name = "frames/";
    if(i < 10) image_name += "frame_00" + std::to_string(i);
    else if(i < 100) image_name += "frame_0" + std::to_string(i);
    else image_name += "frame_" + std::to_string(i);
    image_name += ".ppm";
    std::ofstream image(image_name);

    if((i+1) % 30 == 0){
      x = random_float2();
      y = random_float2();
      z = random_float2();
    }
    float theta = (2.0f * 3.14159f * i) / frames;
    
    float novo_x = lookAt.x + raio * std::sin(theta);
    float novo_z = lookAt.z + raio * std::cos(theta);
    float novo_y = lookAt.y + altura_camera;

    lookFrom = Point4(novo_x, novo_y, novo_z);

    w = (lookFrom - lookAt); 
    w.normalize();

    u = cross(vUp, w); 
    u.normalize();

    v_cam = cross(w, u);

    // transforming the cube
    //cube->applyTranslate(translate(Vector4(-cube->centroid.x, -cube->centroid.y, -cube->centroid.z)));
    //cube->applyRotation(rotate(Vector4(x, y, z, 0), 3.1416/64));
    //cube->applyScale(scale(Vector4(4, 4, 4)));
    //cube->applyTranslate(translate(Vector4(cube->centroid.x, cube->centroid.y, cube->centroid.z)));
    //std::cout << cube->centroid.x << " " << cube->centroid.y << " " << cube->centroid.z << "\n";

    // putting the transformed cube in the world
    //world.push_back(std::move(cube));

    // rendering
    if(image.is_open()) {
      image << "P3\n";
      image << nCol << " " << nLin << "\n";
      image << 255 << "\n";

      raycast(image, 0, 0, nCol, nLin);

      image.close();
    }

    auto stop = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = stop - start;

    std::cout << "Frame " << i+1 << " out of " << frames << " rendered in " << elapsed.count() << " seconds.\n";

    // creating a pointer to the transformed cube
    //std::unique_ptr<Object> temp_generic = std::move(world.back());
    // giving the ownership back to cube
    //cube.reset(static_cast<ListMesh*>(temp_generic.release()));
    // getting the cube out of world so theres always only one cube
    //world.pop_back();
  }
  auto full_stop = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = full_stop - full_start;
  std::cout << elapsed.count() << " seconds to render " << frames << " frames.\n";
}