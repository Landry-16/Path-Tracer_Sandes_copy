/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** box
*/

#include "rt/interfaces/IPrimitive.hpp"
#include "rt/math/Ray.hpp"
#include "rt/math/Vec3.hpp"
#include "rt/core/Factory.hpp"
#include "PrimitiveUtils.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace {

static double axisVal(const Vec3 &v, int i)
{
    return i == 0 ? v.x : (i == 1 ? v.y : v.z);
}

static int dominantAxis(const Vec3 &d)
{
    int ax = 0;
    if (std::abs(d.y) > std::abs(axisVal(d, ax))) ax = 1;
    if (std::abs(d.z) > std::abs(axisVal(d, ax))) ax = 2;
    return ax;
}

}

class Box : public IPrimitive {
public:
    Box(const Vec3 &min, const Vec3 &max, std::shared_ptr<const IMaterial> mat, const Matrix4 &tr)
        : minBound(min), maxBound(max), material(std::move(mat))
        , transform(tr), invTransform(tr.inverse())
    {}

    bool intersect(const Ray &ray, double tMin, double tMax, HitRecord &rec) const override
    {
        double tHit;
        if (!slabTest(ray, tMin, tMax, tHit)) return false;
        rec.t = tHit;
        rec.point = ray.at(tHit);
        Vec3 center = (minBound + maxBound) * 0.5;
        Vec3 size = maxBound - minBound;
        Vec3 lp = rec.point - center;
        Vec3 d = Vec3(lp.x / size.x, lp.y / size.y, lp.z / size.z) * 2.0;
        fillFace(rec, lp, size, d);
        rec.material = material;
        return true;
    }

    bool intersectShadow(const Ray &ray, double tMin, double tMax) const override
    {
        double tHit;
        return slabTest(ray, tMin, tMax, tHit);
    }

    AABB getBounds() const override
    {
        return AABB(minBound, maxBound);
    }

private:
    Vec3 minBound, maxBound;
    std::shared_ptr<const IMaterial> material;
    Matrix4 transform, invTransform;

    bool slabTest(const Ray &ray, double tMin, double tMax, double &tHit) const
    {
        Vec3 inv(1.0 / ray.direction.x, 1.0 / ray.direction.y, 1.0 / ray.direction.z);
        Vec3 t0 = (minBound - ray.origin) * inv;
        Vec3 t1 = (maxBound - ray.origin) * inv;
        Vec3 tN(std::min(t0.x, t1.x), std::min(t0.y, t1.y), std::min(t0.z, t1.z));
        Vec3 tF(std::max(t0.x, t1.x), std::max(t0.y, t1.y), std::max(t0.z, t1.z));
        double nearT = std::max({tN.x, tN.y, tN.z});
        double farT = std::min({tF.x, tF.y, tF.z});
        if (nearT > farT || farT < tMin || nearT > tMax) return false;
        tHit = (nearT > tMin) ? nearT : farT;
        return tHit >= tMin && tHit <= tMax;
    }

// UV axis table: for each dominant axis (0=X,1=Y,2=Z), which local axes provide U and V
    void fillFace(HitRecord &rec, const Vec3 &lp, const Vec3 &size, const Vec3 &d) const
    {
        static constexpr struct { int u, v; } UV[3] = {{2, 1}, {0, 2}, {0, 1}};
        int ax = dominantAxis(d);
        double sign = axisVal(d, ax) > 0 ? 1.0 : -1.0;
        Vec3 normals[3] = {Vec3(sign, 0, 0), Vec3(0, sign, 0), Vec3(0, 0, sign)};
        rec.normal = normals[ax];
        rec.u = axisVal(lp, UV[ax].u) / axisVal(size, UV[ax].u) + 0.5;
        rec.v = axisVal(lp, UV[ax].v) / axisVal(size, UV[ax].v) + 0.5;
    }
};

extern "C" {
    void rt_plugin_register()
    {
        PrimitiveRegistry::instance().registerType("box",
            [](const PrimitiveParams &p) -> std::unique_ptr<IPrimitive> {
                double size = p.radius * 2.0;
                Vec3 halfSize(size * 0.5, size * 0.5, size * 0.5);
                return std::make_unique<Box>(p.position - halfSize, p.position + halfSize, p.material, p.transform);
            }
        );
    }
}
