/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** Plane
*/

#include "rt/primitives/Plane.hpp"
#include <cmath>

Plane::Plane(const Vec3 &point, const Vec3 &normal, std::shared_ptr<const IMaterial> material)
    : point(point), normal(normal.normalized()), material(std::move(material)),
      transform(Matrix4::identity()), invTransform(Matrix4::identity())
{
}

Plane::Plane(const Vec3 &point, const Vec3 &normal, std::shared_ptr<const IMaterial> material, const Matrix4 &transform)
    : point(point), normal(normal.normalized()), material(std::move(material)),
      transform(transform), invTransform(transform.inverse())
{
}

Ray Plane::toLocalRay(const Ray &ray) const
{
    return Ray(
        invTransform.transformPoint(ray.origin),
        invTransform.transformDirection(ray.direction)
    );
}

std::optional<double> Plane::computeT(const Ray &localRay) const
{
    double denom = normal.dot(localRay.direction);
    std::optional<double> result;

    if (std::abs(denom) >= 1e-8)
        result = (point - localRay.origin).dot(normal) / denom;
    return result;
}

std::optional<double> Plane::validateT(double t, double tMin, double tMax) const
{
    std::optional<double> result;

    if (t >= tMin && t <= tMax)
        result = t;
    return result;
}

int Plane::dominantAxis(const Vec3 &vec) const
{
    std::array<double, 3> comps = {vec.x, vec.y, vec.z};
    int axis = 0;

    for (int i = 1; i < 3; ++i) {
        if (comps[i] > comps[axis])
            axis = i;
    }
    return axis;
}

double Plane::componentAt(const Vec3 &vec, int axis) const
{
    std::array<double, 3> comps = {vec.x, vec.y, vec.z};
    return comps[axis];
}

std::pair<double, double> Plane::computeUV(const Vec3 &localPoint) const
{
    static const std::array<std::pair<int, int>, 3> UV_FROM_AXIS = {{
        {2, 1},
        {0, 2},
        {0, 1}
    }};
    Vec3 absNormal(std::abs(normal.x), std::abs(normal.y), std::abs(normal.z));
    int axis = dominantAxis(absNormal);
    const auto &indices = UV_FROM_AXIS[axis];
    double uvScale = 2.0;

    return std::make_pair(componentAt(localPoint, indices.first) / uvScale,
                          componentAt(localPoint, indices.second) / uvScale);
}

void Plane::fillHit(HitRecord &hit, double t, const Ray &localRay) const
{
    Vec3 localPoint = localRay.at(t);
    auto uv = computeUV(localPoint);

    hit.t = t;
    hit.point = transform.transformPoint(localPoint);
    hit.normal = invTransform.transpose().transformDirection(normal).normalized();
    hit.material = material;
    hit.u = uv.first;
    hit.v = uv.second;
}

bool Plane::intersect(const Ray &ray, double tMin, double tMax, HitRecord &hit) const
{
    Ray localRay = toLocalRay(ray);
    auto t = computeT(localRay);
    bool result = false;

    if (t.has_value()) {
        auto valid = validateT(t.value(), tMin, tMax);
        if (valid.has_value()) {
            fillHit(hit, valid.value(), localRay);
            result = true;
        }
    }
    return result;
}

bool Plane::intersectShadow(const Ray &ray, double tMin, double tMax) const
{
    Ray localRay = toLocalRay(ray);
    auto t = computeT(localRay);
    bool result = false;

    if (t.has_value())
        result = validateT(t.value(), tMin, tMax).has_value();
    return result;
}

void Plane::clampAxis(Vec3 &mn, Vec3 &mx, int axis) const
{
    std::array<double *, 3> mnComps = {&mn.x, &mn.y, &mn.z};
    std::array<double *, 3> mxComps = {&mx.x, &mx.y, &mx.z};
    double p = componentAt(point, axis);

    *mnComps[axis] = p - 1.0;
    *mxComps[axis] = p + 1.0;
}

std::array<Vec3, 8> Plane::localCorners(const Vec3 &mn, const Vec3 &mx) const
{
    return {
        Vec3(mn.x, mn.y, mn.z),
        Vec3(mx.x, mn.y, mn.z),
        Vec3(mn.x, mx.y, mn.z),
        Vec3(mx.x, mx.y, mn.z),
        Vec3(mn.x, mn.y, mx.z),
        Vec3(mx.x, mn.y, mx.z),
        Vec3(mn.x, mx.y, mx.z),
        Vec3(mx.x, mx.y, mx.z)
    };
}

AABB Plane::getBounds() const
{
    const double large = 1e6;
    Vec3 absNormal(std::abs(normal.x), std::abs(normal.y), std::abs(normal.z));
    Vec3 localMin = point - Vec3(large, large, large);
    Vec3 localMax = point + Vec3(large, large, large);
    int axis = dominantAxis(absNormal);

    if (componentAt(absNormal, axis) > 0.9)
        clampAxis(localMin, localMax, axis);
    auto corners = localCorners(localMin, localMax);
    AABB worldBounds;
    for (const auto &corner : corners)
        worldBounds = worldBounds.expand(transform.transformPoint(corner));
    return worldBounds;
}
