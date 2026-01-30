#include "../../utils/Rectangle.hpp"
#include <cmath>

Rectangle::Rectangle(const Point4& _p0, const Point4& _p1, const Point4& _p2, 
                     const Point3& _color, const Point3& _diffuse, const Point3& _specular, 
                     float _reflectivity)
    : p0(_p0), color(_color), diffuse(_diffuse), specular(_specular), reflectivity(_reflectivity) 
{
    // Define as arestas baseadas nos pontos
    edge_a = _p1 - _p0;
    edge_b = _p2 - _p0;
    
    // Calcula a normal
    normal = cross(edge_a, edge_b);
    normal.normalize();
}

bool Rectangle::Intersect(const Point4 &origin, const Vector4 &dir, float t_min, float t_max, HitRecord &hr) const {
    // 1. Interseção com o plano infinito (mesma lógica do Plain)
    float denominator = dot(normal, dir);
    
    // Verifica se o raio é paralelo ao plano
    if (std::abs(denominator) < 1e-6) return false;

    float t = dot(p0 - origin, normal) / denominator;

    if (t < t_min || t > t_max) return false;

    // 2. Verifica se o ponto está dentro dos limites do retângulo
    Point4 p_int = origin + t * dir;
    Vector4 d = p_int - p0;

    // Projeta o vetor d sobre as arestas para encontrar as coordenadas locais (u, v)
    // u = (d . edge_a) / |edge_a|^2
    // v = (d . edge_b) / |edge_b|^2
    
    float dot_a = dot(d, edge_a);
    float dot_b = dot(d, edge_b);
    float len_sq_a = dot(edge_a, edge_a);
    float len_sq_b = dot(edge_b, edge_b);

    float u = dot_a / len_sq_a;
    float v = dot_b / len_sq_b;

    // Se u e v estiverem entre 0 e 1, o ponto está dentro do retângulo
    if (u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f) {
        hr.t = t;
        hr.p_int = p_int;
        hr.normal = normal;
        hr.obj_ptr = this;
        hr.uv = Point3(u, v, 0); // Opcional: coordenadas de textura
        return true;
    }

    return false;
}