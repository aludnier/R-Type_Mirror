/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** OverlapTests.cpp
*/

#include <algorithm>
#include <cmath>
#include "OverlapTests.h"

namespace RType::Physics2D {
    namespace {
        float signOf(float v) { return v < 0.f ? -1.f : 1.f; }
    }

    bool circleCircle(const Shape &a, const Shape &b, Vector2 &normal, float &penetration) {
        const Vector2 d = b.center - a.center;
        const float dist = d.length();
        const float radii = a.radius + b.radius;
        if (dist >= radii)
            return false;
        normal = dist > 1e-6f ? d / dist : Vector2{1.f, 0.f};
        penetration = radii - dist;
        return true;
    }

    bool boxBox(const Shape &a, const Shape &b, Vector2 &normal, float &penetration) {
        const Vector2 d = b.center - a.center;
        const float ox = a.half.x + b.half.x - std::abs(d.x);
        const float oy = a.half.y + b.half.y - std::abs(d.y);
        if (ox <= 0.f || oy <= 0.f)
            return false;
        if (ox < oy) {
            normal = {signOf(d.x), 0.f};
            penetration = ox;
        } else {
            normal = {0.f, signOf(d.y)};
            penetration = oy;
        }
        return true;
    }

    bool boxCircle(const Shape &box, const Shape &circle, Vector2 &normalBoxToCircle, float &penetration) {
        const Vector2 rel = circle.center - box.center;
        const Vector2 closest{std::clamp(rel.x, -box.half.x, box.half.x),
                              std::clamp(rel.y, -box.half.y, box.half.y)};
        const Vector2 diff = rel - closest;
        const float dist = diff.length();
        if (dist > 1e-6f) {
            if (dist >= circle.radius)
                return false;
            normalBoxToCircle = diff / dist;
            penetration = circle.radius - dist;
            return true;
        }
        const float px = box.half.x - std::abs(rel.x);
        const float py = box.half.y - std::abs(rel.y);
        if (px < py) {
            normalBoxToCircle = {signOf(rel.x), 0.f};
            penetration = px + circle.radius;
        } else {
            normalBoxToCircle = {0.f, signOf(rel.y)};
            penetration = py + circle.radius;
        }
        return true;
    }

    bool overlap(const Shape &a, const Shape &b, Vector2 &normal, float &penetration) {
        if (a.isBox && b.isBox)
            return boxBox(a, b, normal, penetration);
        if (!a.isBox && !b.isBox)
            return circleCircle(a, b, normal, penetration);
        if (a.isBox)
            return boxCircle(a, b, normal, penetration);
        if (!boxCircle(b, a, normal, penetration))
            return false;
        normal = -normal;
        return true;
    }
}
