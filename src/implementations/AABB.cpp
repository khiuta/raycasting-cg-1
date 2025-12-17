#include "../../utils/AABB.hpp"
#include "../../utils/BVH.hpp"
#include <cmath>

bool AABB::IntersectRayAABB(const Point4& origin, const Vector4& dir, float& t_hit) const {
    // Começa com intervalo infinito
    float t_min = -std::numeric_limits<float>::infinity();
    float t_max = std::numeric_limits<float>::infinity();
    
    // --- EIXO X ---
    if (std::abs(dir.x) < 1e-9f) { 
        // Raio paralelo ao eixo X. Se a origem estiver fora da caixa, nunca acerta.
        if (origin.x < min_x || origin.x > max_x) return false;
    } else {
        float invD = 1.0f / dir.x;
        float t0 = (min_x - origin.x) * invD;
        float t1 = (max_x - origin.x) * invD;
        if (invD < 0.0f) std::swap(t0, t1);
        t_min = t0 > t_min ? t0 : t_min; // std::max
        t_max = t1 < t_max ? t1 : t_max; // std::min
        if (t_max <= t_min) return false;
    }

    // --- EIXO Y ---
    if (std::abs(dir.y) < 1e-9f) {
        if (origin.y < min_y || origin.y > max_y) return false;
    } else {
        float invD = 1.0f / dir.y;
        float t0 = (min_y - origin.y) * invD;
        float t1 = (max_y - origin.y) * invD;
        if (invD < 0.0f) std::swap(t0, t1);
        t_min = t0 > t_min ? t0 : t_min;
        t_max = t1 < t_max ? t1 : t_max;
        if (t_max <= t_min) return false;
    }

    // --- EIXO Z ---
    if (std::abs(dir.z) < 1e-9f) {
        if (origin.z < min_z || origin.z > max_z) return false;
    } else {
        float invD = 1.0f / dir.z;
        float t0 = (min_z - origin.z) * invD;
        float t1 = (max_z - origin.z) * invD;
        if (invD < 0.0f) std::swap(t0, t1);
        t_min = t0 > t_min ? t0 : t_min;
        t_max = t1 < t_max ? t1 : t_max;
        if (t_max <= t_min) return false;
    }

    // Se a caixa está atrás da câmera (t_max < 0), é um erro
    if (t_max < 0) return false;

    // Se t_min for negativo (câmera dentro da caixa), o hit é instantâneo (0)
    t_hit = (t_min < 0.0f) ? 0.0f : t_min;

    return true;
}

int point_inside_aabb(Point4 &p, AABB& aabb){
  if(p.x >= aabb.min_x && p.x <= aabb.max_x){
    if(p.y >= aabb.min_y && p.y <= aabb.max_y){
      if(p.z >= aabb.min_z && p.z <= aabb.max_z) return 1;
    }
  } else return 0;
  return 0;
}

bool triangle_inside_aabb(Triangle &tri, AABB& aabb){
  Point4 centroid = (tri.p1 + tri.p2 + tri.p3) / 3.0f;
  if(point_inside_aabb(centroid, aabb)) return true;
  return false;
}

void AABB::refit() {
    // Começa "avesso" para poder crescer
    min_x = 1e30f; max_x = -1e30f;
    min_y = 1e30f; max_y = -1e30f;
    min_z = 1e30f; max_z = -1e30f;

    for (const auto& tri : t) {
        // Expande para incluir p1
        min_x = std::min(min_x, tri->p1.x); max_x = std::max(max_x, tri->p1.x);
        min_y = std::min(min_y, tri->p1.y); max_y = std::max(max_y, tri->p1.y);
        min_z = std::min(min_z, tri->p1.z); max_z = std::max(max_z, tri->p1.z);

        // Expande para incluir p2
        min_x = std::min(min_x, tri->p2.x); max_x = std::max(max_x, tri->p2.x);
        min_y = std::min(min_y, tri->p2.y); max_y = std::max(max_y, tri->p2.y);
        min_z = std::min(min_z, tri->p2.z); max_z = std::max(max_z, tri->p2.z);

        // Expande para incluir p3
        min_x = std::min(min_x, tri->p3.x); max_x = std::max(max_x, tri->p3.x);
        min_y = std::min(min_y, tri->p3.y); max_y = std::max(max_y, tri->p3.y);
        min_z = std::min(min_z, tri->p3.z); max_z = std::max(max_z, tri->p3.z);
    }
}

void AABB::buildBVH(int depth) {
  float h = this->height();
  float w = this->width();
  float d = this->depth();

  if(depth <= 0) {
    left = nullptr;
    right = nullptr;
    refit();
    return;
  }

  float mid_x = (min_x + max_x) / 2.0f;
    float mid_y = (min_y + max_y) / 2.0f;
    float mid_z = (min_z + max_z) / 2.0f;

    float dx = max_x - min_x;
    float dy = max_y - min_y;
    float dz = max_z - min_z;

    int axis = 0; // 0=X, 1=Y, 2=Z
    if (dy > dx && dy > dz) axis = 1;
    else if (dz > dx && dz > dy) axis = 2;

    // 2. Cria os filhos VAZIOS (sem coordenadas fixas ainda!)
    auto left_node = std::make_unique<AABB>();
    auto right_node = std::make_unique<AABB>();

    // 3. Distribui os triângulos baseado no Centróide
    for (auto* tri : t) {
        Point4 c = (tri->p1 + tri->p2 + tri->p3) / 3.0f;
        bool goes_left = false;
        
        if (axis == 0) goes_left = (c.x < mid_x);
        else if (axis == 1) goes_left = (c.y < mid_y);
        else goes_left = (c.z < mid_z);

        if (goes_left) left_node->t.push_back(tri);
        else right_node->t.push_back(tri);
    }
    
    // CUIDADO: Se um lado ficou vazio, não divida! Senão entra em loop infinito.
    if (left_node->t.empty() || right_node->t.empty()) {
        left = nullptr; right = nullptr;
        refit();
        return;
    }

    // 4. REFIT: A mágica acontece aqui. 
    // As caixas agora se ajustam ao tamanho real dos triângulos dentro delas.
    left_node->refit();
    right_node->refit();

    // 5. Conecta e Recursa
    left = std::move(left_node);
    right = std::move(right_node);
    
    // Limpa os triângulos deste nó (eles desceram para os filhos)
    this->t.clear(); 
    this->t.shrink_to_fit();

    left->buildBVH(depth - 1);
    right->buildBVH(depth - 1);
}

bool AABB::is_leaf() const{
  if(left == nullptr && right == nullptr) return true;
  return false;
}

bool AABB::Hit(const Point4& origin, const Vector4& dir, float t_min, float t_max, HitRecord& rec) const {
    // 1. Se o raio não toca nesta caixa, nem perde tempo com o que tem dentro.
    float t_box;
    if (!IntersectRayAABB(origin, dir, t_box)) return false; 

    // 2. SE FOR FOLHA: Testa os triângulos (Força bruta local)
    if (is_leaf()) {
        bool hit_anything = false;
        float closest_so_far = t_max;

        for (const auto& tri : t) {
            HitRecord temp_rec;
            // Se bater no triângulo E for mais perto que o anterior...
            if (tri->Intersect(origin, dir, t_min, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t; // Atualiza o limite para o próximo
                rec = temp_rec;              // Salva o registro
            }
        }
        return hit_anything;
    }

    // 3. SE FOR NÓ INTERNO: Testa os filhos
    bool hit_left = false;
    bool hit_right = false;

    // Tenta a esquerda
    if (left) {
        hit_left = left->Hit(origin, dir, t_min, t_max, rec);
    }

    // Tenta a direita
    if (right) {
        // OTIMIZAÇÃO: Se bateu na esquerda em t=5, só precisamos testar a direita
        // se ela estiver mais perto que 5.
        float t_limit = hit_left ? rec.t : t_max;
        
        HitRecord rec_right;
        if (right->Hit(origin, dir, t_min, t_limit, rec_right)) {
            rec = rec_right; // Direita ganhou (era mais próxima)
            hit_right = true;
        }
    }

    return hit_left || hit_right;
}