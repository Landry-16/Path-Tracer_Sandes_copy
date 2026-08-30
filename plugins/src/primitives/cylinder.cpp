/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** cylinder
*/

#include <memory>
#include "rt/interfaces/IPrimitive.hpp"
#include "rt/core/Factory.hpp"
#include "rt/math/Vec3.hpp"
#include "PrimitiveUtils.hpp"
#include "HitContext.hpp"
#include <cmath>

class Cylinder : public IPrimitive {
public:
    Cylinder(const Vec3 &center, double radius, double height, std::shared_ptr<const IMaterial> material, const Matrix4 &transform)
        : center(center), radius(radius), height(height), material(std::move(material)),
          transform(transform), invTransform(transform.inverse())
    {}

    bool intersect(const Ray &ray, double tMin, double tMax, HitRecord &hit) const override
    {
        Ray lr(invTransform.transformPoint(ray.origin),
               invTransform.transformDirection(ray.direction));
        HitContext ctx(tMax);
        checkCylinderBodyHit(lr, tMin, ctx);
        Vec3 upN(0, 1, 0), downN(0, -1, 0);
        checkCylinderCapHit(lr, center.y + height / 2.0, upN, tMin, ctx);
        checkCylinderCapHit(lr, center.y - height / 2.0, downN, tMin, ctx);
        if (!ctx.hit) return false;
        hit.t = ctx.closestT;
        hit.point = transform.transformPoint(ctx.point);
        hit.normal = invTransform.transpose().transformDirection(ctx.normal).normalized();
        hit.material = material;
        hit.u = hit.v = 0.0;
        return true;
    }

    bool intersectShadow(const Ray &ray, double tMin, double tMax) const override
    {
        Ray lr(invTransform.transformPoint(ray.origin),
               invTransform.transformDirection(ray.direction));
        if (shadowHitBody(lr, tMin, tMax)) return true;
        return shadowHitCap(lr, center.y + height / 2.0, tMin, tMax)
            || shadowHitCap(lr, center.y - height / 2.0, tMin, tMax);
    }

    AABB getBounds() const override
    {
        Vec3 r(radius, height / 2.0, radius);
        return utils::transformLocalAABB(transform, center - r, center + r);
    }

private:
    Vec3 center;
    double radius;
    double height;
    std::shared_ptr<const IMaterial> material;
    Matrix4 transform;
    Matrix4 invTransform;

    void tryBodyHitAtT(const Ray &lr, double t, double tMin, HitContext &ctx) const
    {
        if (t < tMin || t >= ctx.closestT) return;
        Vec3 p = lr.at(t);
        double y = p.y - center.y;
        if (y < -height / 2.0 || y > height / 2.0) return;
        ctx.closestT = t;
        ctx.point = p;
        ctx.normal = Vec3(p.x - center.x, 0, p.z - center.z).normalized();
        ctx.hit = true;
    }

    void checkCylinderBodyHit(const Ray &lr, double tMin, HitContext &ctx) const
    {
        Vec3 oc = lr.origin - center;
        double a = lr.direction.x * lr.direction.x + lr.direction.z * lr.direction.z;
        if (std::abs(a) <= 1e-8) return;
        double b = 2.0 * (oc.x * lr.direction.x + oc.z * lr.direction.z);
        double c = oc.x * oc.x + oc.z * oc.z - radius * radius;
        double disc = b * b - 4 * a * c;
        if (disc < 0) return;
        double sqrtd = std::sqrt(disc);
        tryBodyHitAtT(lr, (-b - sqrtd) / (2.0 * a), tMin, ctx);
        tryBodyHitAtT(lr, (-b + sqrtd) / (2.0 * a), tMin, ctx);
    }

    void checkCylinderCapHit(const Ray &lr, double yPlane, const Vec3 &capNormal,
                              double tMin, HitContext &ctx) const
    {
        if (std::abs(lr.direction.y) <= 1e-8) return;
        double t = (yPlane - lr.origin.y) / lr.direction.y;
        if (t < tMin || t >= ctx.closestT) return;
        Vec3 p = lr.at(t);
        double dx = p.x - center.x, dz = p.z - center.z;
        if (dx * dx + dz * dz > radius * radius) return;
        ctx.closestT = t;
        ctx.point = p;
        ctx.normal = capNormal;
        ctx.hit = true;
    }

    bool shadowHitBody(const Ray &lr, double tMin, double tMax) const
    {
        Vec3 oc = lr.origin - center;
        double a = lr.direction.x * lr.direction.x + lr.direction.z * lr.direction.z;
        if (std::abs(a) <= 1e-8) return false;
        double b = 2.0 * (oc.x * lr.direction.x + oc.z * lr.direction.z);
        double c = oc.x * oc.x + oc.z * oc.z - radius * radius;
        double disc = b * b - 4 * a * c;
        if (disc < 0) return false;
        double sqrtd = std::sqrt(disc);
        for (double t : {(-b - sqrtd) / (2.0 * a), (-b + sqrtd) / (2.0 * a)}) {
            if (t < tMin || t > tMax) continue;
            double y = lr.at(t).y - center.y;
            if (y >= -height / 2.0 && y <= height / 2.0) return true;
        }
        return false;
    }

    bool shadowHitCap(const Ray &lr, double yPlane,
                      double tMin, double tMax) const
    {
        if (std::abs(lr.direction.y) <= 1e-8) return false;
        double t = (yPlane - lr.origin.y) / lr.direction.y;
        if (t < tMin || t > tMax) return false;
        Vec3 p = lr.at(t);
        double dx = p.x - center.x, dz = p.z - center.z;
        return dx * dx + dz * dz <= radius * radius;
    }
};

extern "C" {
    void rt_plugin_register()
    {
        PrimitiveRegistry::instance().registerType("cylinder",
            [](const PrimitiveParams &p) -> std::unique_ptr<IPrimitive> {
                return std::make_unique<Cylinder>(p.position, p.radius, p.radius * 2.0, p.material, p.transform);
            }
        );
    }
}
