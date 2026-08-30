/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** IPrimitive
*/

#ifndef RT_IPRIMITIVE_HPP
    #define RT_IPRIMITIVE_HPP

    #include "rt/math/Vec3.hpp"
    #include "rt/math/Ray.hpp"
    #include "rt/math/AABB.hpp"
    #include <memory>

class IMaterial;

struct HitRecord {
    Vec3 point;
    Vec3 normal;
    double t;
    std::shared_ptr<const IMaterial> material;
    double u;
    double v;
};

class IPrimitive {
public:
    virtual ~IPrimitive() = default;
    virtual bool intersect(const Ray &ray, double tMin, double tMax, HitRecord &hit) const = 0;
    virtual bool intersectShadow(const Ray &ray, double tMin, double tMax) const = 0;
    virtual AABB getBounds() const = 0;
};

#endif // RT_IPRIMITIVE_HPP
