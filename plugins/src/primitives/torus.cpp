/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** torus
*/

#include <memory>
#include "rt/interfaces/IPrimitive.hpp"
#include "rt/core/Factory.hpp"
#include "rt/math/Vec3.hpp"
#include "PrimitiveUtils.hpp"
#include <cmath>

class Torus : public IPrimitive {
public:
    Torus(const Vec3 &center, double majorRadius, double minorRadius, std::shared_ptr<const IMaterial> material, const Matrix4 &transform)
        : center(center), majorRadius(majorRadius), minorRadius(minorRadius), material(std::move(material)),
          transform(transform), invTransform(transform.inverse())
    {}
    
    double signedDistance(const Vec3 &p) const
    {
        Vec3 localP = p - center;
        double qx = std::sqrt(localP.x * localP.x + localP.z * localP.z) - majorRadius;
        double qy = localP.y;
        return std::sqrt(qx * qx + qy * qy) - minorRadius;
    }
    
    bool intersect(const Ray &ray, double tMin, double tMax, HitRecord &hit) const override
    {
        Ray lr(invTransform.transformPoint(ray.origin),
               invTransform.transformDirection(ray.direction));
        Vec3 rd = lr.direction.normalized();
        double t = tMin;
        const int maxSteps = 200;
        const double epsilon = 0.001;
        for (int i = 0; i < maxSteps && t < tMax; i++) {
            Vec3 pos = lr.origin + rd * t;
            double dist = signedDistance(pos);
            if (std::abs(dist) < epsilon) {
                fillTorusHit(hit, pos, t, pos - center);
                return true;
            }
            t += std::abs(dist);
        }
        return false;
    }
    
    bool intersectShadow(const Ray &ray, double tMin, double tMax) const override
    {
        HitRecord hit;
        return intersect(ray, tMin, tMax, hit);
    }
    
    AABB getBounds() const override
    {
        double outerR = majorRadius + minorRadius;
        Vec3 lo = center - Vec3(outerR, minorRadius, outerR);
        Vec3 hi = center + Vec3(outerR, minorRadius, outerR);
        return utils::transformLocalAABB(transform, lo, hi);
    }

private:
    Vec3 center;
    double majorRadius;
    double minorRadius;
    std::shared_ptr<const IMaterial> material;
    Matrix4 transform;
    Matrix4 invTransform;

    Vec3 computeTorusNormal(const Vec3 &localPos) const
    {
        double x = localPos.x, y = localPos.y, z = localPos.z;
        double distXZ = std::sqrt(x * x + z * z);
        if (distXZ > 1e-8) {
            double f = 1.0 - majorRadius / distXZ;
            return Vec3(x * f, y, z * f).normalized();
        }
        return Vec3(0, y > 0 ? 1 : -1, 0);
    }

    void fillTorusHit(HitRecord &hit, const Vec3 &pos, double t,
                      const Vec3 &localPos) const
    {
        double x = localPos.x, y = localPos.y, z = localPos.z;
        double distXZ = std::sqrt(x * x + z * z);
        Vec3 normal = computeTorusNormal(localPos);
        hit.t = t;
        hit.point = transform.transformPoint(pos);
        hit.normal = invTransform.transpose().transformDirection(normal).normalized();
        hit.material = material;
        double theta = std::atan2(z, x);
        double phi = std::atan2(y, distXZ - majorRadius);
        hit.u = (theta + M_PI) / (2.0 * M_PI);
        hit.v = (phi + M_PI) / (2.0 * M_PI);
    }
};

extern "C" {
    void rt_plugin_register() {
        PrimitiveRegistry::instance().registerType(
            "torus",
            [](const PrimitiveParams &p) {
                double majorRadius = p.radius;
                double minorRadius = p.direction.x > 0 ? p.direction.x : majorRadius * 0.3;
                return std::make_unique<Torus>(p.position, majorRadius, minorRadius, p.material, p.transform);
            }
        );
    }
}
