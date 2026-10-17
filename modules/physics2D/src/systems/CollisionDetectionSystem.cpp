/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** CollisionDetectionSystem.cpp
*/

#include "ecs/SparseArray.h"
#include "CollisionDetectionSystem.h"
#include "OverlapTests.h"
#include "../components/BoxCollider2D.h"
#include "../components/CircleCollider2D.h"
#include "components/Transform2D.h"

namespace RType::Physics2D {
    namespace {
        Shape makeBoxShape(std::size_t id, Vector2 position, const BoxCollider2D &c) {
            return {id, position + c.offset, c.size * 0.5f, 0.f, true, c.isTrigger, c.layer, c.mask};
        }

        Shape makeCircleShape(std::size_t id, Vector2 position, const CircleCollider2D &c) {
            return {id, position + c.offset, {}, c.radius, false, c.isTrigger, c.layer, c.mask};
        }

        template<typename Boxes, typename Circles, typename Transforms>
        std::vector<Shape> buildShapes(Transforms &transforms, Boxes &boxes, Circles &circles) {
            std::vector<Shape> shapes;
            for (std::size_t i = 0; i < transforms.size(); ++i) {
                if (!transforms[i].has_value())
                    continue;
                const Vector2 position = transforms[i]->position;
                if (i < boxes.size() && boxes[i].has_value())
                    shapes.push_back(makeBoxShape(i, position, *boxes[i]));
                else if (i < circles.size() && circles[i].has_value())
                    shapes.push_back(makeCircleShape(i, position, *circles[i]));
            }
            return shapes;
        }

        bool makeContact(const Shape &a, const Shape &b, Contact &out) {
            if (!canCollide(a, b))
                return false;
            Vector2 normal;
            float penetration = 0.f;
            if (!overlap(a, b, normal, penetration))
                return false;
            out = {{a.id, b.id}, normal, penetration, a.isTrigger || b.isTrigger};
            return true;
        }

        void findContacts(const std::vector<Shape> &shapes, ContactBuffer &out) {
            for (std::size_t i = 0; i < shapes.size(); ++i)
                for (std::size_t j = i + 1; j < shapes.size(); ++j) {
                    Contact contact;
                    if (makeContact(shapes[i], shapes[j], contact))
                        out.push_back(contact);
                }
        }
    }

    void addCollisionDetectionSystem(Registry &registry, const SharedContacts &contacts) {
        registry.addSystem<Transform2D, BoxCollider2D, CircleCollider2D>(
            [contacts](auto &transforms, auto &boxes, auto &circles) {
                contacts->clear();
                findContacts(buildShapes(transforms, boxes, circles), *contacts);
            });
    }
}
