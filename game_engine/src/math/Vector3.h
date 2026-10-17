#ifndef VECTOR3_H
#define VECTOR3_H

namespace RType {
    struct Vector3 {
        float x = 0.f;
        float y = 0.f;
        float z = 0.f;

        constexpr Vector3() = default;
        constexpr Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

        constexpr Vector3 operator+(const Vector3 &o) const { return {x + o.x, y + o.y, z + o.z}; }
        constexpr Vector3 operator-(const Vector3 &o) const { return {x - o.x, y - o.y, z - o.z}; }
        constexpr Vector3 operator-() const { return {-x, -y, -z}; }
        constexpr Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
        constexpr Vector3 operator/(float s) const { return {x / s, y / s, z / s}; }
        constexpr Vector3 operator*(const Vector3 &o) const { return {x * o.x, y * o.y, z * o.z}; }

        constexpr Vector3 &operator+=(const Vector3 &o) { x += o.x; y += o.y; z += o.z; return *this; }
        constexpr Vector3 &operator-=(const Vector3 &o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
        constexpr Vector3 &operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
        constexpr Vector3 &operator/=(float s) { x /= s; y /= s; z /= s; return *this; }

        constexpr bool operator==(const Vector3 &o) const = default;
    };

    constexpr Vector3 operator*(float s, const Vector3 &v) { return v * s; }
}

#endif
