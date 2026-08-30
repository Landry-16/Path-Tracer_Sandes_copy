/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** triangle
*/

#include <memory>
#include "rt/interfaces/IPrimitive.hpp"
#include "rt/core/Factory.hpp"
#include "rt/math/Vec3.hpp"
#include "PrimitiveUtils.hpp"
#include <cmath>
#include <algorithm>

class Triangle : public IPrimitive {
public:
    Triangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2, std::shared_ptr<const IMaterial> material, const Matrix4 &transform)
        : v0(v0), v1(v1), v2(v2), material(std::move(material)),
          transform(transform), invTransform(transform.inverse())
    {
        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;
        faceNormal = edge1.cross(edge2).normalized();
    }
    
    bool intersect(const Ray &ray, double tMin, double tMax, HitRecord &hit) const override
    {
        Ray lr(invTransform.transformPoint(ray.origin),
               invTransform.transformDirection(ray.direction));
        double t, u, v;
        if (!utils::mollerTrumbore(lr, v0, v1, v2, tMin, tMax, t, u, v))
            return false;
        hit.t = t;
        hit.point = transform.transformPoint(lr.at(t));
        hit.normal = invTransform.transpose().transformDirection(faceNormal).normalized();
        hit.material = material;
        hit.u = hit.v = 0.0;
        return true;
    }

    bool intersectShadow(const Ray &ray, double tMin, double tMax) const override
    {
        HitRecord dummy;
        return intersect(ray, tMin, tMax, dummy);
    }
    
    AABB getBounds() const override
    {
        AABB result;
        for (const auto &corner : {v0, v1, v2})
            result = result.expand(transform.transformPoint(corner));
        return result;
    }

private:
    Vec3 v0, v1, v2;
    Vec3 faceNormal;
    std::shared_ptr<const IMaterial> material;    Matrix4 transform;
    Matrix4 invTransform;
};

extern "C" {
    void rt_plugin_register()
    {
        PrimitiveRegistry::instance().registerType("triangle", 
            [](const PrimitiveParams &p) -> std::unique_ptr<IPrimitive> {
                Vec3 v0 = p.position + Vec3(-p.radius * 0.5, 0, -p.radius * 0.5);
                Vec3 v1 = p.position + Vec3(p.radius * 0.5, 0, -p.radius * 0.5);
                Vec3 v2 = p.position + Vec3(0, p.radius, 0);
                return std::make_unique<Triangle>(v0, v1, v2, p.material, p.transform);
            }
        );
    }
}
