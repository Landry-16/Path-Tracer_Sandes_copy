/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** Triangle geometry implementation
*/

#include "ObjLoader/OBJMesh.hpp"
#include <cmath>
#include <algorithm>

static void computeBoundsForTriangle(Triangle *tri)
{
    Vec3 minPoint(
        std::min({tri->v0.x, tri->v1.x, tri->v2.x}),
        std::min({tri->v0.y, tri->v1.y, tri->v2.y}),
        std::min({tri->v0.z, tri->v1.z, tri->v2.z})
    );
    Vec3 maxPoint(
        std::max({tri->v0.x, tri->v1.x, tri->v2.x}),
        std::max({tri->v0.y, tri->v1.y, tri->v2.y}),
        std::max({tri->v0.z, tri->v1.z, tri->v2.z})
    );
    tri->bounds = AABB(minPoint, maxPoint);
}

Triangle::Triangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2,
    std::shared_ptr<const IMaterial> mat)
    : v0(v0), v1(v1), v2(v2), material(std::move(mat)),
      uv0(), uv1(), uv2(), hasVertexNormals(false)
{
    Vec3 edge1 = v1 - v0;
    Vec3 edge2 = v2 - v0;
    normal = edge1.cross(edge2).normalized();
    n0 = n1 = n2 = normal;
    computeBoundsForTriangle(this);
}

Triangle::Triangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2,
    const Vec2 &uv0, const Vec2 &uv1, const Vec2 &uv2,
    std::shared_ptr<const IMaterial> mat)
    : v0(v0), v1(v1), v2(v2), material(std::move(mat)),
      uv0(uv0), uv1(uv1), uv2(uv2), hasVertexNormals(false)
{
    Vec3 edge1 = v1 - v0;
    Vec3 edge2 = v2 - v0;
    normal = edge1.cross(edge2).normalized();
    n0 = n1 = n2 = normal;
    computeBoundsForTriangle(this);
}

Triangle::Triangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2,
    const Vec3 &n0, const Vec3 &n1, const Vec3 &n2,
    const Vec2 &uv0, const Vec2 &uv1, const Vec2 &uv2,
    std::shared_ptr<const IMaterial> mat)
    : v0(v0), v1(v1), v2(v2), material(std::move(mat)),
      n0(n0), n1(n1), n2(n2), uv0(uv0), uv1(uv1), uv2(uv2),
      hasVertexNormals(true)
{
    Vec3 edge1 = v1 - v0;
    Vec3 edge2 = v2 - v0;
    normal = edge1.cross(edge2).normalized();
    computeBoundsForTriangle(this);
}

static bool mollerTrumbore(const Ray &ray, const Vec3 &v0,
    const Vec3 &e1, const Vec3 &e2,
    double &t, double &u, double &v)
{
    const double EPSILON = 1e-8;
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

static Vec3 interpolateNormal(const Triangle &tri, double u, double v)
{
    if (tri.hasVertexNormals)
        return (tri.n0 * (1.0 - u - v) + tri.n1 * u + tri.n2 * v).normalized();
    return tri.normal;
}

bool Triangle::intersect(const Ray &ray, double tMin, double tMax,
    HitRecord &hit) const
{
    Vec3 e1 = v1 - v0;
    Vec3 e2 = v2 - v0;
    double t, u, v;
    if (!mollerTrumbore(ray, v0, e1, e2, t, u, v))
        return false;
    if (t < tMin || t > tMax)
        return false;
    hit.t = t;
    hit.point = ray.at(t);
    Vec3 interp = interpolateNormal(*this, u, v);
    bool frontFace = ray.direction.dot(interp) < 0;
    hit.normal = frontFace ? interp : (interp * -1.0);
    hit.material = material;
    Vec2 texCoord = uv0 * (1.0 - u - v) + uv1 * u + uv2 * v;
    hit.u = texCoord.x;
    hit.v = texCoord.y;
    return true;
}

bool Triangle::intersectShadow(const Ray &ray, double tMin, double tMax) const
{
    HitRecord tmp;
    return intersect(ray, tMin, tMax, tmp);
}

AABB Triangle::getBounds() const
{
    return bounds;
}
