#ifndef POINT4
#define POINT4
#include "Vector4.hpp"

class Point4 {
    public:
    float x, y, z, w;

    Point4() : x(0), y(0), z(0), w(1) {};
    Point4(float x, float y, float z) : x(x), y(y), z(z), w(1) {};
    Point4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {};
};

// point + vector
inline Point4 operator+(const Point4 &p, const Vector4 &v){
    return { p.x + v.x, p.y + v.y, p.z + v.z, p.w + v.w };
}
inline Point4 operator+(const Vector4 &v, const Point4 &p){
    return { p.x + v.x, p.y + v.y, p.z + v.z, p.w + v.w };
}
// point - point
inline Vector4 operator-(const Point4& p1, const Point4 &p2){
    return { p1.x - p2.x, p1.y - p2.y, p1.z - p2.z, p1.w - p2.w };
}
// point + point
inline Point4 operator+(const Point4& p1, const Point4& p2){
    return { p1.x + p2.x, p1.y + p2.y, p1.z + p2.z, 1 };
}
// point / float
inline Point4 operator/(const Point4& p1, float f){
    return { p1.x / f, p1.y / f, p1.z / f, 1 };
}
#endif