/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** PathTracer
*/

#ifndef RT_PATHTRACER_HPP
    #define RT_PATHTRACER_HPP

    #include <memory>
    #include <random>
    #include "rt/scene/Scene.hpp"
    #include "rt/math/Color.hpp"
    #include "rt/rendering/BVH.hpp"
    #include "rt/math/Ray.hpp"

namespace rt {
    class ProgressiveDisplay;
}

class PathTracer {
public:
    static std::vector<Color> render(const Scene &scene, int samplesPerPixel = 100);
    static std::vector<Color> renderWithDisplay(const Scene &scene,
                                                std::shared_ptr<rt::ProgressiveDisplay> display,
                                                int samplesPerPixel = 100);

private:
    struct TraceCtx
    {
        const BVH &bvh;
        const std::vector<std::unique_ptr<ILight>> &lights;
        int depth;
        std::mt19937 &rng;

        TraceCtx next() const { return {bvh, lights, depth + 1, rng}; }
    };

    struct SampleCtx
    {
        std::mt19937 &rng;
        std::uniform_real_distribution<double> &dist;
        int samplesPerPixel;
    };

    static Color trace(const Ray &ray, TraceCtx ctx);

    static Color sampleDirectLighting(const HitRecord &hit, const Vec3 &wo,
                                      const BVH &bvh,
                                      const std::vector<std::unique_ptr<ILight>> &lights);

    static Vec3  randomHemisphere(const Vec3 &normal, std::mt19937 &rng);
    static Vec3  randomUnitVector(std::mt19937 &rng);
    static double randomDouble(std::mt19937 &rng);

    static Color tracePBR(const Ray &ray, const HitRecord &hit,
                          TraceCtx ctx, const Color &direct);
    static Color traceTransparent(const Ray &ray, const HitRecord &hit, TraceCtx ctx);
    static Color traceReflective(const Ray &ray, const HitRecord &hit,
                                 TraceCtx ctx, const Color &direct);
    static Color traceDiffuse(const HitRecord &hit, TraceCtx ctx,
                              const Color &direct, double rrProb);

    static Color samplePixel(const Scene &scene, const BVH &bvh,
                             int i, int j, SampleCtx &ctx);

    static void renderTileWorker(const Scene &scene,
                                 const BVH &bvh,
                                 std::vector<Color> &pixels,
                                 class TileScheduler &scheduler,
                                 int samplesPerPixel,
                                 std::shared_ptr<rt::ProgressiveDisplay> display);
};

#endif // RT_PATHTRACER_HPP
