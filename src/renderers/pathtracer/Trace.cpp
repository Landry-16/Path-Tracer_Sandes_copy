/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** PathTracer : trace dispatch and material shading variants
*/

#include "rt/rendering/PathTracer.hpp"
#include "rt/interfaces/ITexture.hpp"
#include <cmath>
#include <limits>


static Color skyColor(const Ray &ray)
{
    double t = 0.5 * (ray.direction.normalized().y + 1.0);
    return Color(1.0, 1.0, 1.0) * (1.0 - t) + Color(0.5, 0.7, 1.0) * t;
}

static double fresnelSchlick(double cosi, double ior)
{
    double r0 = (1.0 - ior) / (1.0 + ior);
    r0 *= r0;
    return r0 + (1.0 - r0) * std::pow(1.0 - cosi, 5.0);
}

struct TransmissionData
{
    Vec3   reflDir;
    Vec3   refrDir;
    double fresnel;
    bool   tir;
};

static TransmissionData computeTransmission(const Vec3 &rayDir,
    const Vec3 &hitNormal, double ior)
{
    bool   entering = rayDir.dot(hitNormal) < 0;
    Vec3   n   = entering ? hitNormal : hitNormal * -1.0;
    double eta = entering ? 1.0 / ior : ior;
    double cosi = std::abs(rayDir.dot(n));
    double k    = 1.0 - eta * eta * (1.0 - cosi * cosi);
    Vec3   reflDir = rayDir - n * 2.0 * rayDir.dot(n);
    if (k < 0)
        return {reflDir, Vec3(), fresnelSchlick(cosi, ior), true};
    return {reflDir, rayDir * eta - n * (eta * cosi + std::sqrt(k)),
            fresnelSchlick(cosi, ior), false};
}

Color PathTracer::tracePBR(const Ray &ray, const HitRecord &hit,
    TraceCtx ctx, const Color &direct)
{
    Vec3 wo = (ray.direction * -1.0).normalized();
    ScatterResult scatter = hit.material->scatter(
        wo, hit.normal, hit.u, hit.v, randomDouble(ctx.rng), randomDouble(ctx.rng));
    if (scatter.valid && scatter.pdf > 0.000001)
        return direct + scatter.attenuation
               * trace(Ray(hit.point, scatter.direction), ctx.next());
    return direct;
}

Color PathTracer::traceTransparent(const Ray &ray, const HitRecord &hit, TraceCtx ctx)
{
    auto [reflDir, refrDir, fresnel, tir] = computeTransmission(
        ray.direction.normalized(), hit.normal, hit.material->getRefractiveIndex());
    Color reflColor = trace(Ray(hit.point, reflDir), ctx.next());
    if (tir)
        return reflColor;
    return reflColor * fresnel
         + trace(Ray(hit.point, refrDir), ctx.next()) * (1.0 - fresnel);
}

Color PathTracer::traceReflective(const Ray &ray, const HitRecord &hit,
    TraceCtx ctx, const Color &direct)
{
    Vec3  reflected = ray.direction - hit.normal * 2.0 * ray.direction.dot(hit.normal);
    Color reflColor = trace(Ray(hit.point, reflected.normalized()), ctx.next());
    double refl     = hit.material->getReflectivity();
    return direct * (1.0 - refl) + reflColor * refl;
}

Color PathTracer::traceDiffuse(const HitRecord &hit, TraceCtx ctx,
    const Color &direct, double rrProb)
{
    Vec3  dir      = randomHemisphere(hit.normal, ctx.rng);
    Color indirect = trace(Ray(hit.point, dir), ctx.next());
    double cosTheta = std::max(0.0, hit.normal.dot(dir));
    auto  texture  = hit.material->getTexture();
    Color diffuse  = texture ? texture->sample(hit.u, hit.v)
                             : hit.material->getDiffuse();
    return direct + diffuse * (1.0 / M_PI) * indirect * cosTheta * (2.0 * M_PI / rrProb);
}

Color PathTracer::trace(const Ray &ray, TraceCtx ctx)
{
    if (ctx.depth >= 8)
        return Color(0, 0, 0);
    HitRecord hit;
    if (!ctx.bvh.intersect(ray, 0.001, std::numeric_limits<double>::infinity(), hit))
        return skyColor(ray);
    if (!hit.material)
        return Color(0, 0, 0);
    Color emission = hit.material->getEmission();
    if (emission.x > 0.01 || emission.y > 0.01 || emission.z > 0.01)
        return emission;
    Vec3  wo     = (ray.direction * -1.0).normalized();
    Color direct = sampleDirectLighting(hit, wo, ctx.bvh, ctx.lights);
    double rrProb = 1.0;
    if (ctx.depth > 3)
    {
        rrProb = 0.8;
        if (randomDouble(ctx.rng) > rrProb)
            return direct;
    }
    if (hit.material->isPBR())
        return tracePBR(ray, hit, ctx, direct);
    if (hit.material->isTransparent())
        return traceTransparent(ray, hit, ctx);
    if (hit.material->isReflective())
        return traceReflective(ray, hit, ctx, direct);
    return traceDiffuse(hit, ctx, direct, rrProb);
}
