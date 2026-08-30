/*
** EPITECH PROJECT, 2026
** local-Raytracer
** File description:
** stl triangle
*/

#include <algorithm>
#include <cmath>
#include "StlLoader/STLMesh.hpp"

namespace {

constexpr double EPSILON = 1e-8;

bool mollerTrumbore(const Ray &ray, const Vec3 &v0,
    const Vec3 &e1, const Vec3 &e2, double &t, double &u, double &v)
{
    Vec3 h = ray.direction.cross(e2);
    double a = e1.dot(h);
    if (std::abs(a) < EPSILON)
        return false;
    double f = 1.0 / a;
    Vec3 s = ray.origin - v0;
    u = f * s.dot(h);
    if (u < 0.0 || u > 1.0)
        return false;
    Vec3 q = s.cross(e1);
    v = f * ray.direction.dot(q);
    if (v < 0.0 || u + v > 1.0)
        return false;
    t = f * e2.dot(q);
    return true;
}

}

STLTriangle::STLTriangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2,
    std::shared_ptr<const IMaterial> mat)
    : _v0(v0), _v1(v1), _v2(v2),
      _material(std::move(mat))
{
    _normal = (v1 - v0).cross(v2 - v0).normalized();
    computeBounds();
}

STLTriangle::STLTriangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2,
    const Vec3 &fileNormal, std::shared_ptr<const IMaterial> mat)
    : _v0(v0), _v1(v1), _v2(v2),
      _material(std::move(mat))
{
    if (fileNormal.lengthSquared() > EPSILON)
        _normal = fileNormal.normalized();
    else
        _normal = (v1 - v0).cross(v2 - v0).normalized();
    computeBounds();
}

void STLTriangle::computeBounds()
{
    Vec3 lo(
        std::min({_v0.x, _v1.x, _v2.x}),
        std::min({_v0.y, _v1.y, _v2.y}),
        std::min({_v0.z, _v1.z, _v2.z})
    );
    Vec3 hi(
        std::max({_v0.x, _v1.x, _v2.x}),
        std::max({_v0.y, _v1.y, _v2.y}),
        std::max({_v0.z, _v1.z, _v2.z})
    );
    _bounds = AABB(lo, hi);
}

bool STLTriangle::isDegenerate() const
{
    return (_v1 - _v0).cross(_v2 - _v0).lengthSquared() < EPSILON;
}

bool STLTriangle::intersect(const Ray &ray, double tMin, double tMax,
    HitRecord &hit) const
{
    Vec3 e1 = _v1 - _v0;
    Vec3 e2 = _v2 - _v0;
    double t, u, v;
    if (!mollerTrumbore(ray, _v0, e1, e2, t, u, v))
        return false;
    if (t < tMin || t > tMax)
        return false;
    hit.t = t;
    hit.point = ray.at(t);
    hit.material = _material;
    bool frontFace = ray.direction.dot(_normal) < 0;
    hit.normal = frontFace ? _normal : (_normal * -1.0);
    hit.u = u;
    hit.v = v;
    return true;
}

bool STLTriangle::intersectShadow(const Ray &ray, double tMin,
    double tMax) const
{
    Vec3 e1 = _v1 - _v0;
    Vec3 e2 = _v2 - _v0;
    double t, u, v;
    if (!mollerTrumbore(ray, _v0, e1, e2, t, u, v))
        return false;
    return (t >= tMin && t <= tMax);
}

AABB STLTriangle::getBounds() const
{
    return _bounds;
}
