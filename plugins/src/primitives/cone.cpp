/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** cone
*/

#include <memory>
#include "rt/interfaces/IPrimitive.hpp"
#include "rt/core/Factory.hpp"
#include "rt/math/Vec3.hpp"
#include "PrimitiveUtils.hpp"
#include "HitContext.hpp"
#include <cmath>

class Cone : public IPrimitive {
public:
    Cone(const Vec3 &apex, double radius, double height, std::shared_ptr<const IMaterial> material, const Matrix4 &transform)
        : apex(apex), radius(radius), height(height), material(std::move(material)),
          transform(transform), invTransform(transform.inverse())
    {}

    bool intersect(const Ray &ray, double tMin, double tMax, HitRecord &hit) const override
    {
        Ray lr(invTransform.transformPoint(ray.origin),
               invTransform.transformDirection(ray.direction));
        HitContext ctx(tMax);
        Vec3 baseCenter = apex - Vec3(0, height, 0);
        double k = (radius * radius) / (height * height);
        checkConeLateralHit(lr, k, tMin, ctx);
        checkConeBaseHit(lr, baseCenter, tMin, ctx);
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
        Vec3 baseCenter = apex - Vec3(0, height, 0);
        double k = (radius * radius) / (height * height);
        return shadowHitLateral(lr, k, tMin, tMax)
            || shadowHitBase(lr, baseCenter, tMin, tMax);
    }

    AABB getBounds() const override
    {
        Vec3 baseCenter = apex - Vec3(0, height, 0);
        Vec3 r(radius, 0, radius);
        Vec3 lo = baseCenter - r;
        Vec3 hi = Vec3(apex.x + radius, apex.y, apex.z + radius);
        return utils::transformLocalAABB(transform, lo, hi);
    }

private:
    Vec3 apex;
    double radius;
    double height;
    std::shared_ptr<const IMaterial> material;
    Matrix4 transform;
    Matrix4 invTransform;

    void computeConeQuadratic(const Ray &lr, double k,
                              double &a, double &b, double &c) const
    {
        Vec3 co = lr.origin - apex;
        a = lr.direction.x * lr.direction.x + lr.direction.z * lr.direction.z
            - k * lr.direction.y * lr.direction.y;
        b = 2.0 * (co.x * lr.direction.x + co.z * lr.direction.z
                   - k * co.y * lr.direction.y);
        c = co.x * co.x + co.z * co.z - k * co.y * co.y;
    }

    void tryConeLateralHitAtT(const Ray &lr, double t, double tMin, HitContext &ctx) const
    {
        if (t < tMin || t >= ctx.closestT) return;
        Vec3 p = lr.at(t);
        double y = apex.y - p.y;
        if (y < 0 || y > height) return;
        ctx.closestT = t;
        ctx.point = p;
        Vec3 radial(p.x - apex.x, 0, p.z - apex.z);
        if (radial.length() > 1e-8) radial = radial.normalized();
        ctx.normal = Vec3(radial.x, radius / height, radial.z).normalized();
        ctx.hit = true;
    }

    void checkConeLateralHit(const Ray &lr, double k, double tMin, HitContext &ctx) const
    {
        double a, b, c;
        computeConeQuadratic(lr, k, a, b, c);
        double disc = b * b - 4.0 * a * c;
        if (disc < 0 || std::abs(a) <= 1e-8) return;
        double sqrtd = std::sqrt(disc);
        tryConeLateralHitAtT(lr, (-b - sqrtd) / (2.0 * a), tMin, ctx);
        tryConeLateralHitAtT(lr, (-b + sqrtd) / (2.0 * a), tMin, ctx);
    }

    void checkConeBaseHit(const Ray &lr, const Vec3 &baseCenter, double tMin, HitContext &ctx) const
    {
        if (std::abs(lr.direction.y) <= 1e-8) return;
        double t = (baseCenter.y - lr.origin.y) / lr.direction.y;
        if (t < tMin || t >= ctx.closestT) return;
        Vec3 p = lr.at(t);
        double dx = p.x - baseCenter.x, dz = p.z - baseCenter.z;
        if (dx * dx + dz * dz > radius * radius) return;
        ctx.closestT = t;
        ctx.point = p;
        ctx.normal = Vec3(0, -1, 0);
        ctx.hit = true;
    }

    bool shadowHitLateral(const Ray &lr, double k,
                          double tMin, double tMax) const
    {
        double a, b, c;
        computeConeQuadratic(lr, k, a, b, c);
        double disc = b * b - 4.0 * a * c;
        if (disc < 0 || std::abs(a) <= 1e-8) return false;
        double sqrtd = std::sqrt(disc);
        for (double t : {(-b - sqrtd) / (2.0 * a), (-b + sqrtd) / (2.0 * a)}) {
            if (t < tMin || t > tMax) continue;
            double y = apex.y - lr.at(t).y;
            if (y >= 0 && y <= height) return true;
        }
        return false;
    }

    bool shadowHitBase(const Ray &lr, const Vec3 &baseCenter,
                       double tMin, double tMax) const
    {
        if (std::abs(lr.direction.y) <= 1e-8) return false;
        double t = (baseCenter.y - lr.origin.y) / lr.direction.y;
        if (t < tMin || t > tMax) return false;
        Vec3 p = lr.at(t);
        double dx = p.x - baseCenter.x, dz = p.z - baseCenter.z;
        return dx * dx + dz * dz <= radius * radius;
    }
};

extern "C" {
    void rt_plugin_register()
    {
        PrimitiveRegistry::instance().registerType("cone",
            [](const PrimitiveParams &p) -> std::unique_ptr<IPrimitive> {
                Vec3 apex = p.position + Vec3(0, p.radius * 2.0, 0);
                return std::make_unique<Cone>(apex, p.radius, p.radius * 2.0, p.material, p.transform);
            }
        );
    }
}
