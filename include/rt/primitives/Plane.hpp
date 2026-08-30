/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** Plane
*/

#ifndef RT_PLANE_HPP
    #define RT_PLANE_HPP

    #include "rt/interfaces/IPrimitive.hpp"
    #include "rt/interfaces/IMaterial.hpp"
    #include "rt/math/Matrix4.hpp"
    #include <array>
    #include <optional>
    #include <utility>

class Plane : public IPrimitive {
public:
    Plane(const Vec3 &point, const Vec3 &normal, std::shared_ptr<const IMaterial> material);
    Plane(const Vec3 &point, const Vec3 &normal, std::shared_ptr<const IMaterial> material, const Matrix4 &transform);

    bool intersect(const Ray &ray, double tMin, double tMax, HitRecord &hit) const override;
    bool intersectShadow(const Ray &ray, double tMin, double tMax) const override;
    AABB getBounds() const override;

private:
    Ray toLocalRay(const Ray &ray) const;
    std::optional<double> computeT(const Ray &localRay) const;
    std::optional<double> validateT(double t, double tMin, double tMax) const;
    int dominantAxis(const Vec3 &vec) const;
    double componentAt(const Vec3 &vec, int axis) const;
    std::pair<double, double> computeUV(const Vec3 &localPoint) const;
    void fillHit(HitRecord &hit, double t, const Ray &localRay) const;
    void clampAxis(Vec3 &mn, Vec3 &mx, int axis) const;
    std::array<Vec3, 8> localCorners(const Vec3 &mn, const Vec3 &mx) const;

    Vec3 point;
    Vec3 normal;
    std::shared_ptr<const IMaterial> material;
    Matrix4 transform;
    Matrix4 invTransform;
};

#endif // RT_PLANE_HPP
