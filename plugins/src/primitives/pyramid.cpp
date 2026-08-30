/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** pyramid
*/

#include <memory>
#include <vector>
#include <algorithm>
#include "rt/interfaces/IPrimitive.hpp"
#include "rt/core/Factory.hpp"
#include "rt/math/Vec3.hpp"
#include "PrimitiveUtils.hpp"
#include "HitContext.hpp"
#include <cmath>

class TriangleFace {
public:
    Vec3 v0, v1, v2;
    Vec3 normal;

    TriangleFace(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2)
        : v0(v0), v1(v1), v2(v2)
    {
        normal = (v1 - v0).cross(v2 - v0).normalized();
    }

    bool intersect(const Ray &ray, double tMin, double tMax, double &t) const
    {
        double u, v;
        return utils::mollerTrumbore(ray, v0, v1, v2, tMin, tMax, t, u, v);
    }
};

class Pyramid : public IPrimitive {
public:
    Pyramid(const Vec3 &base, double size, double height, std::shared_ptr<const IMaterial> material, const Matrix4 &transform)
        : baseCenter(base), size(size), height(height), material(std::move(material)),
          transform(transform), invTransform(transform.inverse())
    {
        Vec3 apex = baseCenter + Vec3(0, height, 0);
        double half = size * 0.5;

        Vec3 b0 = baseCenter + Vec3(-half, 0, -half);
        Vec3 b1 = baseCenter + Vec3(half, 0, -half);
        Vec3 b2 = baseCenter + Vec3(half, 0, half);
        Vec3 b3 = baseCenter + Vec3(-half, 0, half);

        faces.emplace_back(b0, b1, apex);
        faces.emplace_back(b1, b2, apex);
        faces.emplace_back(b2, b3, apex);
        faces.emplace_back(b3, b0, apex);
        faces.emplace_back(b0, b2, b1);
        faces.emplace_back(b0, b3, b2);
    }

    bool intersect(const Ray &ray, double tMin, double tMax, HitRecord &hit) const override
    {
        Ray lr(invTransform.transformPoint(ray.origin),
               invTransform.transformDirection(ray.direction));
        HitContext ctx(tMax);
        if (!findClosestFaceHit(lr, tMin, ctx)) return false;
        fillHitRecord(hit, ctx.closestT, ctx.point, ctx.normal);
        return true;
    }

    bool intersectShadow(const Ray &ray, double tMin, double tMax) const override
    {
        Ray lr(invTransform.transformPoint(ray.origin),
               invTransform.transformDirection(ray.direction));
        return std::any_of(faces.begin(), faces.end(),
            [&](const TriangleFace &f) { double t; return f.intersect(lr, tMin, tMax, t); });
    }

    AABB getBounds() const override
    {
        double half = size * 0.5;
        Vec3 apex = baseCenter + Vec3(0, height, 0);
        const Vec3 corners[5] = {
            baseCenter + Vec3(-half, 0, -half), baseCenter + Vec3(half, 0, -half),
            baseCenter + Vec3(half, 0, half), baseCenter + Vec3(-half, 0, half),
            apex
        };
        AABB result;
        for (const auto &c : corners)
            result = result.expand(transform.transformPoint(c));
        return result;
    }

private:
    Vec3 baseCenter;
    double size;
    double height;
    std::shared_ptr<const IMaterial> material;
    Matrix4 transform, invTransform;
    std::vector<TriangleFace> faces;

    bool findClosestFaceHit(const Ray &lr, double tMin, HitContext &ctx) const
    {
        for (const auto &face : faces) {
            double t;
            if (!face.intersect(lr, tMin, ctx.closestT, t)) continue;
            ctx.closestT = t;
            ctx.point = lr.at(t);
            ctx.normal = face.normal;
            ctx.hit = true;
        }
        return ctx.hit;
    }

    void fillHitRecord(HitRecord &hit, double t, const Vec3 &lp, const Vec3 &n) const
    {
        hit.t = t;
        hit.point = transform.transformPoint(lp);
        hit.normal = invTransform.transpose().transformDirection(n).normalized();
        hit.material = material;
        hit.u = hit.v = 0.0;
    }
};

extern "C" {
    void rt_plugin_register()
    {
        PrimitiveRegistry::instance().registerType("pyramid",
            [](const PrimitiveParams &p) -> std::unique_ptr<IPrimitive> {
                return std::make_unique<Pyramid>(p.position, p.radius * 2.0, p.radius * 1.5, p.material, p.transform);
            }
        );
    }
}
