/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** PathTracer : random sampling utilities and direct lighting
*/

#include "rt/rendering/PathTracer.hpp"
#include "rt/lights/AmbientLight.hpp"
#include "rt/interfaces/ITexture.hpp"
#include <cmath>
#include <optional>

Vec3 PathTracer::randomUnitVector(std::mt19937 &rng)
{
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double z = dist(rng) * 2.0 - 1.0;
    double a = dist(rng) * 2.0 * M_PI;
    double r = std::sqrt(1.0 - z * z);
    return Vec3(r * std::cos(a), r * std::sin(a), z);
}

Vec3 PathTracer::randomHemisphere(const Vec3 &normal, std::mt19937 &rng)
{
    Vec3 v = randomUnitVector(rng);
    return (v.dot(normal) > 0.0) ? v : (v * -1.0);
}

double PathTracer::randomDouble(std::mt19937 &rng)
{
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng);
}

struct LightRay { Vec3 dir; double dist; };

static std::optional<LightRay> computeLightRay(const HitRecord &hit,
    const std::unique_ptr<ILight> &light)
{
    Vec3 dir;
    double dist;
    if (light->isDirectional())
    {
        dir  = (light->getDirection(hit.point) * -1.0).normalized();
        dist = 1e10;
    }
    else
    {
        Vec3 toLight = light->getDirection(hit.point);
        dist = toLight.length();
        if (dist < 1e-6)
            return std::nullopt;
        dir = toLight.normalized();
    }
    if (std::isnan(dir.x) || std::isnan(dir.y) || std::isnan(dir.z))
        return std::nullopt;
    return LightRay{dir, dist};
}

static Color evalBRDF(const HitRecord &hit, const Vec3 &wo, const Vec3 &lightDir)
{
    if (hit.material->isPBR())
        return hit.material->evaluateBRDF(wo, lightDir, hit.normal, hit.u, hit.v);
    auto texture = hit.material->getTexture();
    Color diffuse = texture ? texture->sample(hit.u, hit.v) : hit.material->getDiffuse();
    return diffuse * (1.0 / M_PI);
}

static Color computeOneLightContrib(const HitRecord &hit, const Vec3 &wo,
    const BVH &bvh, const std::unique_ptr<ILight> &light)
{
    auto lr = computeLightRay(hit, light);
    if (!lr)
        return Color(0, 0, 0);
    double cosTheta = std::max(0.0, hit.normal.dot(lr->dir));
    if (cosTheta < 0.000001)
        return Color(0, 0, 0);
    HitRecord shadowHit;
    if (bvh.intersect(Ray(hit.point, lr->dir), 0.001, lr->dist - 0.001, shadowHit))
        return Color(0, 0, 0);
    Color intensity = light->getIntensity();
    if (!light->isDirectional())
        intensity = intensity / std::max(lr->dist * lr->dist, 0.01);
    return evalBRDF(hit, wo, lr->dir) * intensity * cosTheta;
}

Color PathTracer::sampleDirectLighting(const HitRecord &hit, const Vec3 &wo,
    const BVH &bvh, const std::vector<std::unique_ptr<ILight>> &lights)
{
    Color total(0, 0, 0);
    for (const auto &light : lights)
    {
        if (dynamic_cast<const AmbientLight*>(light.get()))
            continue;
        total += computeOneLightContrib(hit, wo, bvh, light);
    }
    return total;
}
