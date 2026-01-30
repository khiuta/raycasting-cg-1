#include "../../utils/Cone.hpp"
#include <cmath>
#include <algorithm>

Cone::Cone(Point4 c_base, float r, bool bottom, Point4 v, Point3 col, Point3 dif, Point3 spec)
    : center(c_base), radius(r), has_bottom(bottom), vertice(v), 
      color(col), diffuse_color(dif), specular_color(spec) {
    
    Vector4 axis = vertice - center;
    this->height = axis.length();
    this->dc = axis;
    this->dc.normalize();
}

bool Cone::Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const {
    Vector4 w = origin - vertice; // Vetor da origem ao vértice (topo)
    
    // Relação geométrica: cos²(theta) / sin²(theta)
    float k = (radius / height);
    float m = k * k;

    // Equação quadrática para cone alinhado a um eixo arbitrário dc
    // a = (dir.dc)² - cos²(alpha)*|dir|² -> Simplificado via projeção:
    float dot_dir_dc = dot(dir, dc);
    Vector4 dir_proj = dir - dc * dot_dir_dc;
    float a = dot_dir_dc * dot_dir_dc - m * dot(dir_proj, dir_proj);

    float dot_w_dc = dot(w, dc);
    Vector4 w_proj = w - dc * dot_w_dc;
    float b = 2.0f * (dot_dir_dc * dot_w_dc - m * dot(dir_proj, w_proj));

    float c = dot_w_dc * dot_w_dc - m * dot(w_proj, w_proj);

    float disc = b * b - 4.0f * a * c;
    if (disc < 0) return false;

    float sqrt_disc = std::sqrt(disc);
    float t1 = (-b - sqrt_disc) / (2.0f * a);
    float t2 = (-b + sqrt_disc) / (2.0f * a);

    float t_hit = -1.0f;
    Vector4 final_normal;

    // Testar t1 e t2 dentro dos limites de altura
    for (float t : {t1, t2}) {
        if (t < t_min || t > t_max) continue;

        Point4 p_int = origin + dir * t;
        Vector4 v_to_p = p_int - vertice;
        float dist_on_axis = dot(v_to_p, dc);

        // O ponto deve estar entre o vértice (0) e a base (-height se dc for base->vértice)
        // Como dc é base->vértice, o vértice está em height e a base em 0.
        // v_to_p . dc deve ser negativo (em direção à base) entre 0 e -height.
        if (dist_on_axis <= 0 && dist_on_axis >= -height) {
            if (t_hit < 0 || t < t_hit) {
                t_hit = t;
                
                // Cálculo da Normal: 
                // A normal em um ponto P de um cone é ortogonal à geratriz.
                Vector4 cp = p_int - center;
                Vector4 orthogonal_to_axis = cp - dc * dot(cp, dc);
                orthogonal_to_axis.normalize();
                
                // Inclinação da normal baseada no ângulo do cone
                float tan_alpha = radius / height;
                final_normal = orthogonal_to_axis + dc * tan_alpha;
                final_normal.normalize();
            }
        }
    }

    // Interseção com a Base (Disco)
    if (has_bottom) {
        float denom = dot(dir, dc);
        if (std::abs(denom) > 1e-6) {
            float t_b = dot(center - origin, dc) / denom;
            if (t_b > t_min && t_b < t_max && (t_hit < 0 || t_b < t_hit)) {
                Point4 p_b = origin + dir * t_b;
                if ((p_b - center).length() <= radius) {
                    t_hit = t_b;
                    final_normal = -dc; // Normal da base aponta para fora
                }
            }
        }
    }

    if (t_hit > 0) {
        hr.t = t_hit;
        hr.p_int = origin + dir * t_hit;
        hr.normal = final_normal;
        hr.obj_ptr = this;
        // Garantir que a normal aponte contra o raio (lado externo)
        if (dot(hr.normal, dir) > 0) hr.normal = -hr.normal;
        return true;
    }

    return false;
}