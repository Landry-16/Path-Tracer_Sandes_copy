/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** Sphere
*/

#ifndef RT_SPHERE_HPP
    #define RT_SPHERE_HPP

    #include "rt/interfaces/IPrimitive.hpp"
    #include "rt/interfaces/IMaterial.hpp"
    #include "rt/math/Matrix4.hpp"
    #include <array>
    #include <optional>
    #include <utility>

class Sphere : public IPrimitive {
public:
    Sphere(const Vec3 &center, double radius, std::shared_ptr<const IMaterial> material);
    Sphere(const Vec3 &center, double radius, std::shared_ptr<const IMaterial> material, const Matrix4 &transform);

    bool intersect(const Ray &ray, double tMin, double tMax, HitRecord &hit) const override;
    bool intersectShadow(const Ray &ray, double tMin, double tMax) const override;
    AABB getBounds() const override;

private:
    Ray toLocalRay(const Ray &ray) const;
    std::optional<std::pair<double, double>> solveQuadratic(const Ray &localRay) const;
    std::optional<double> pickValidRoot(const std::pair<double, double> &roots, double tMin, double tMax) const;
    std::pair<double, double> computeUV(const Vec3 &localNormal) const;
    void fillHit(HitRecord &hit, double root, const Ray &localRay) const;
    std::array<Vec3, 8> localCorners(const AABB &localBounds) const;

    Vec3 center;
    double radius;
    std::shared_ptr<const IMaterial> material;
    Matrix4 transform;
    Matrix4 invTransform;
};

#endif // RT_SPHERE_HPP
