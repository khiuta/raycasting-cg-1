#include "../../utils/Triangle.hpp"
#include <iostream>
#include <cmath>

float random_float() {
    static std::mt19937 generator(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()
    );
    
    static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    
    return distribution(generator);
}

Triangle::Triangle(const Point4 &p1, const Point4 &p2, const Point4 &p3, const Vector4 &n){
  this->p1 = p1;
  this->p2 = p2;
  this->p3 = p3;
  this->normal = n;
  this->normal.normalize();
  Vector4 r1 = p2 - p1;
  Vector4 r2 = p3 - p1;
  this->area = cross(r1, r2).length()/2;
  Vector4 e2 = p3 - p2;
  Vector4 e3 = p3 - p1;
  this->e1 = r1;
  this->e2 = e2;
  this->e3 = e3;
  float r = 0.4f;
  float g = 0.4f;
  float b = 0.4f;
  this->color = Point3(r, g, b);
  this->dif_color = Point3(r, g, b);
  this->spec_color = Point3(.7, .7, .7);
}

Triangle::Triangle(const Point4 &p1, const Point4 &p2, const Point4 &p3, const Vector4 &n, const Point3 &vt1, const Point3 &vt2, const Point3 &vt3){
  this->p1 = p1;
  this->p2 = p2;
  this->p3 = p3;
  this->vt1 = vt1;
  this->vt2 = vt2;
  this->vt3 = vt3;
  this->normal = n;
  this->normal.normalize();
  Vector4 r1 = p2 - p1;
  Vector4 r2 = p3 - p1;
  this->area = cross(r1, r2).length()/2;
  Vector4 e2 = p3 - p2;
  Vector4 e3 = p3 - p1;
  this->e1 = r1;
  this->e2 = e2;
  this->e3 = e3;
  float r = 0.4f;
  float g = 0.4f;
  float b = 0.4f;
  this->color = Point3(r, g, b);
  this->dif_color = Point3(r, g, b);
  this->spec_color = Point3(.7, .7, .7);
}

// ... (includes e código anterior mantém igual) ...

bool Triangle::Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const {
  if(dot(normal, dir) >= 0) return false;
  
  float denominator = dot(this->normal, dir);
  double t = 0;

  if(std::abs(denominator) > 0.0001f){
    Vector4 p1_to_origin = this->p1 - origin;
    t = dot(p1_to_origin, this->normal)/denominator; 
  } else return false;

  // Verifica intervalo válido antes de cálculos pesados
  if (t <= t_min || t >= t_max) return false;

  Point4 p_int = origin + t*dir;

  Vector4 s1 = p_int - p1;
  Vector4 s2 = p_int - p2;
  Vector4 s3 = p_int - p3;

  double c1 = dot(this->normal, cross(s3, s1))/(2*this->area);
  double c2 = dot(this->normal, cross(s1, s2))/(2*this->area);
  double c3 = 1.0 - c2 - c1;

  if(c1 < 0 || c2 < 0 || c3 < 0){
    return false;
  } 

  // --- LÓGICA DE TEXTURA E TRANSPARÊNCIA ---
  
  Point3 uv;
  // Interpolação baricêntrica das coordenadas de textura
  uv.x = c3*vt1.x + c1*vt2.x + c2*vt3.x;
  uv.y = c3*vt1.y + c1*vt2.y + c2*vt3.y;

  // Se houver uma malha e uma textura associada, verificamos o Alpha
  if (this->mesh != nullptr && this->mesh->texture != nullptr && !this->mesh->texture->colors.empty()) {
      Texture* tex = this->mesh->texture;
      
      float u_val = uv.x - std::floor(uv.x);
      float v_val = 1.0f - (uv.y - std::floor(uv.y)); // Invertendo V (padrão OpenGL)

      int tex_u = (int)(u_val * (tex->width - 1));
      int tex_v = (int)(v_val * (tex->height - 1));

      // Clamp por segurança
      tex_u = std::max(0, std::min(tex_u, tex->width - 1));
      tex_v = std::max(0, std::min(tex_v, tex->height - 1));

      // Pegamos o canal Alfa (o quarto elemento da tupla: índice 3)
      uint8_t alpha = std::get<3>(tex->colors[tex_v][tex_u]);

      // Limiar de transparência (Cutout threshold)
      // Se for muito transparente (< 10 de 255), ignoramos a colisão
      if (alpha < 10) {
          return false; // O raio passa direto!
      }
  }

  // Se chegou aqui, colidiu com algo sólido
  hr.normal = this->normal;
  if(dot(hr.normal, dir) > 0){
    hr.normal = -hr.normal;
  }
  hr.t = t;
  hr.p_int = p_int;
  hr.obj_ptr = this;
  hr.uv = uv;
  
  if (this->mesh != nullptr) {
      hr.texture = this->mesh->texture;
  } else {
      hr.texture = nullptr;
  }

  return true;
}
void Triangle::applyTranslate(const Matrix4 &m){
  Vector4 newP1 = m * Vector4(p1.x, p1.y, p1.z, 1.0f);
  Vector4 newP2 = m * Vector4(p2.x, p2.y, p2.z, 1.0f);
  Vector4 newP3 = m * Vector4(p3.x, p3.y, p3.z, 1.0f);

  p1 = Point4(newP1.x, newP1.y, newP1.z, 1.0f);
  p2 = Point4(newP2.x, newP2.y, newP2.z, 1.0f);
  p3 = Point4(newP3.x, newP3.y, newP3.z, 1.0f);

  this->e1 = p2 - p1;
  this->e2 = p3 - p2;
  this->e3 = p3 - p1;

  Vector4 newNormal = m * Vector4(normal.x, normal.y, normal.z, 0.0f);
  this->normal = normalize(newNormal);

  this->area = cross(this->e1, -this->e3).length() / 2.0;
}

void Triangle::applyScale(const Matrix4 &m){
  Vector4 newP1 = m * Vector4(p1.x, p1.y, p1.z, 1.0f);
  Vector4 newP2 = m * Vector4(p2.x, p2.y, p2.z, 1.0f);
  Vector4 newP3 = m * Vector4(p3.x, p3.y, p3.z, 1.0f);

  p1 = Point4(newP1.x, newP1.y, newP1.z, 1.0f);
  p2 = Point4(newP2.x, newP2.y, newP2.z, 1.0f);
  p3 = Point4(newP3.x, newP3.y, newP3.z, 1.0f);

  this->e1 = p2 - p1;
  this->e2 = p3 - p2;
  this->e3 = p3 - p1;

  this->normal.x /= m.cols[0].x;
  this->normal.y /= m.cols[1].y;
  this->normal.z /= m.cols[2].z;

  this->area = cross(this->e1, -this->e3).length() / 2.0;

  this->normal.normalize();
}

void Triangle::applyRotation(const Matrix4 &m){
  Vector4 newP1 = m * Vector4(p1.x, p1.y, p1.z, 1.0f);
  Vector4 newP2 = m * Vector4(p2.x, p2.y, p2.z, 1.0f);
  Vector4 newP3 = m * Vector4(p3.x, p3.y, p3.z, 1.0f);

  p1 = Point4(newP1.x, newP1.y, newP1.z, 1.0f);
  p2 = Point4(newP2.x, newP2.y, newP2.z, 1.0f);
  p3 = Point4(newP3.x, newP3.y, newP3.z, 1.0f);

  this->e1 = p2 - p1;
  this->e2 = p3 - p2;
  this->e3 = p3 - p1;

  Vector4 newNormal = m * Vector4(normal.x, normal.y, normal.z, 0.0f);
  this->normal = normalize(newNormal);

  this->area = cross(this->e1, -this->e3).length() / 2.0;
}

void Triangle::applyShear(const Matrix4 &m) {
    // Aplica a matriz de cisalhamento aos vértices (w = 1.0f)
    Vector4 newP1 = m * Vector4(p1.x, p1.y, p1.z, 1.0f);
    Vector4 newP2 = m * Vector4(p2.x, p2.y, p2.z, 1.0f);
    Vector4 newP3 = m * Vector4(p3.x, p3.y, p3.z, 1.0f);

    p1 = Point4(newP1.x, newP1.y, newP1.z, 1.0f);
    p2 = Point4(newP2.x, newP2.y, newP2.z, 1.0f);
    p3 = Point4(newP3.x, newP3.y, newP3.z, 1.0f);

    // Recalcula as arestas com as novas posições
    this->e1 = p2 - p1;
    this->e2 = p3 - p2;
    this->e3 = p3 - p1;

    // Para a normal (w = 0.0f), o cisalhamento exige cuidado.
    // Idealmente usa-se a transposta da inversa para normais, 
    // mas se a matriz m for apenas o cisalhamento puro:
    Vector4 newNormal = m * Vector4(normal.x, normal.y, normal.z, 0.0f);
    this->normal = normalize(newNormal);

    // Recalcula a área, pois o cisalhamento altera a geometria
    this->area = cross(this->e1, -this->e3).length() / 2.0;
}