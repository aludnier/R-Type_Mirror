#ifndef VECTOR2_H
#define VECTOR2_H

#include <cmath>

namespace RType {
struct Vector2 {
    float x = 0.f;
    float y = 0.f;

    constexpr Vector2() = default;

    constexpr Vector2(float x, float y) :
        x(x), y(y)
    {
    }

    constexpr Vector2 operator+(const Vector2 &o) const
    {
        return {x + o.x, y + o.y};
    }

    constexpr Vector2 operator-(const Vector2 &o) const
    {
        return {x - o.x, y - o.y};
    }

    constexpr Vector2 operator-() const
    {
        return {-x, -y};
    }

    constexpr Vector2 operator*(float s) const
    {
        return {x * s, y * s};
    }

    constexpr Vector2 operator/(float s) const
    {
        return {x / s, y / s};
    }

    constexpr Vector2 operator*(const Vector2 &o) const
    {
        return {x * o.x, y * o.y};
    }

    constexpr Vector2 &operator+=(const Vector2 &o)
    {
        x += o.x;
        y += o.y;
        return *this;
    }

    constexpr Vector2 &operator-=(const Vector2 &o)
    {
        x -= o.x;
        y -= o.y;
        return *this;
    }

    constexpr Vector2 &operator*=(float s)
    {
        x *= s;
        y *= s;
        return *this;
    }

    constexpr Vector2 &operator/=(float s)
    {
        x /= s;
        y /= s;
        return *this;
    }

    constexpr bool operator==(const Vector2 &o) const = default;

    [[nodiscard]] constexpr float dot(const Vector2 &o) const
    {
        return x * o.x + y * o.y;
    }

    [[nodiscard]] constexpr float lengthSquared() const
    {
        return dot(*this);
    }

    [[nodiscard]] float length() const
    {
        return std::sqrt(lengthSquared());
    }
};

constexpr Vector2 operator*(float s, const Vector2 &v)
{
    return v * s;
}
} // namespace RType

#endif
