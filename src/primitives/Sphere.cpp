/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** Sphere
*/

#include "rt/primitives/Sphere.hpp"
#include <cmath>

Sphere::Sphere(const Vec3 &center, double radius,
    std::shared_ptr<const IMaterial> material)
    : center(center), radius(radius), material(std::move(material)),
      transform(Matrix4::identity()), invTransform(Matrix4::identity())
{
}

Sphere::Sphere(const Vec3 &center, double radius,
    std::shared_ptr<const IMaterial> material, const Matrix4 &transform)
    : center(center), radius(radius), material(std::move(material)),
      transform(transform), invTransform(transform.inverse())
{
}

Ray Sphere::toLocalRay(const Ray &ray) const
{
    return Ray(
        invTransform.transformPoint(ray.origin),
        invTransform.transformDirection(ray.direction)
    );
}

std::optional<std::pair<double, double>> Sphere::solveQuadratic(
    const Ray &localRay) const
{
    Vec3 oc = localRay.origin - center;
    double a = localRay.direction.dot(localRay.direction);
    double half_b = oc.dot(localRay.direction);
    double c = oc.dot(oc) - radius * radius;
    double discriminant = half_b * half_b - a * c;
    std::optional<std::pair<double, double>> result;

    if (discriminant >= 0) {
        double sqrtd = std::sqrt(discriminant);
        result = std::make_pair((-half_b - sqrtd) / a, (-half_b + sqrtd) / a);
    }
    return result;
}

std::optional<double> Sphere::pickValidRoot(
    const std::pair<double, double> &roots, double tMin, double tMax) const
{
    std::optional<double> result;

    if (roots.first >= tMin && roots.first <= tMax)
        result = roots.first;
    else if (roots.second >= tMin && roots.second <= tMax)
        result = roots.second;
    return result;
}

std::pair<double, double> Sphere::computeUV(const Vec3 &localNormal) const
{
    double u = 0.5 - std::atan2(localNormal.z, localNormal.x) / (2.0 * M_PI);
    double v = 0.5 + std::asin(localNormal.y) / M_PI;
    return std::make_pair(u, v);
}

void Sphere::fillHit(HitRecord &hit, double root, const Ray &localRay) const
{
    Vec3 localPoint = localRay.at(root);
    Vec3 localNormal = (localPoint - center) / radius;
    auto uv = computeUV(localNormal);

    hit.t = root;
    hit.point = transform.transformPoint(localPoint);
    hit.normal = invTransform.transpose().transformDirection(
                    localNormal).normalized();
    hit.material = material;
    hit.u = uv.first;
    hit.v = uv.second;
}

bool Sphere::intersect(const Ray &ray, double tMin, double tMax,
    HitRecord &hit) const
{
    Ray localRay = toLocalRay(ray);
    auto roots = solveQuadratic(localRay);
    bool result = false;

    if (roots.has_value()) {
        auto root = pickValidRoot(roots.value(), tMin, tMax);
        if (root.has_value()) {
            fillHit(hit, root.value(), localRay);
            result = true;
        }
    }
    return result;
}

bool Sphere::intersectShadow(const Ray &ray, double tMin, double tMax) const
{
    Ray localRay = toLocalRay(ray);
    auto roots = solveQuadratic(localRay);
    bool result = false;

    if (roots.has_value())
        result = pickValidRoot(roots.value(), tMin, tMax).has_value();
    return result;
}

std::array<Vec3, 8> Sphere::localCorners(const AABB &localBounds) const
{
    return {
        Vec3(localBounds.min.x, localBounds.min.y, localBounds.min.z),
        Vec3(localBounds.max.x, localBounds.min.y, localBounds.min.z),
        Vec3(localBounds.min.x, localBounds.max.y, localBounds.min.z),
        Vec3(localBounds.max.x, localBounds.max.y, localBounds.min.z),
        Vec3(localBounds.min.x, localBounds.min.y, localBounds.max.z),
        Vec3(localBounds.max.x, localBounds.min.y, localBounds.max.z),
        Vec3(localBounds.min.x, localBounds.max.y, localBounds.max.z),
        Vec3(localBounds.max.x, localBounds.max.y, localBounds.max.z)
    };
}

AABB Sphere::getBounds() const
{
    Vec3 r(radius, radius, radius);
    AABB localBounds(center - r, center + r);
    auto corners = localCorners(localBounds);
    AABB worldBounds;

    for (const auto &corner : corners)
        worldBounds = worldBounds.expand(transform.transformPoint(corner));
    return worldBounds;
}
