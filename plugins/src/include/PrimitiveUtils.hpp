/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** Shared utility functions for primitive plugins
*/

#ifndef RT_PLUGINS_PRIMITIVE_UTILS_HPP
#define RT_PLUGINS_PRIMITIVE_UTILS_HPP

#include "rt/math/AABB.hpp"
#include "rt/math/Matrix4.hpp"
#include "rt/math/Vec3.hpp"
#include "rt/math/Ray.hpp"
#include <cmath>

namespace utils {

// Transform an axis-aligned bounding box by a matrix, returning the new AABB.
inline AABB transformLocalAABB(const Matrix4 &tr, const Vec3 &lo, const Vec3 &hi)
{
    const Vec3 corners[8] = {
        {lo.x, lo.y, lo.z}, {hi.x, lo.y, lo.z},
        {lo.x, hi.y, lo.z}, {hi.x, hi.y, lo.z},
        {lo.x, lo.y, hi.z}, {hi.x, lo.y, hi.z},
        {lo.x, hi.y, hi.z}, {hi.x, hi.y, hi.z}
    };
    AABB result;
    for (const auto &c : corners)
        result = result.expand(tr.transformPoint(c));
    return result;
}

// Moller–Trumbore ray-triangle intersection.
// Returns true and sets t, u, v if the ray hits triangle (a,b,c) in [tMin,tMax].
inline bool mollerTrumbore(const Ray &ray, const Vec3 &a, const Vec3 &b, const Vec3 &c,
                            double tMin, double tMax,
                            double &t, double &u, double &v)
{
    const double EPS = 1e-8;
    Vec3 e1 = b - a;
    Vec3 e2 = c - a;
    Vec3 h = ray.direction.cross(e2);
    double det = e1.dot(h);
    if (std::abs(det) < EPS) return false;
    double f = 1.0 / det;
    Vec3 s = ray.origin - a;
    u = f * s.dot(h);
    if (u < 0.0 || u > 1.0) return false;
    Vec3 q = s.cross(e1);
    v = f * ray.direction.dot(q);
    if (v < 0.0 || u + v > 1.0) return false;
    t = f * e2.dot(q);
    return t >= tMin && t <= tMax;
}

} // namespace utils

#endif
