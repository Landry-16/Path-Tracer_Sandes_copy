/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** Renderer
*/

#ifndef RT_RENDERER_HPP
    #define RT_RENDERER_HPP

    #include <memory>
    #include <random>
    #include <vector>
    #include "rt/scene/Scene.hpp"
    #include "rt/math/Color.hpp"
    #include "rt/rendering/BVH.hpp"

struct Tile;

namespace rt {
    class ProgressiveDisplay;
}

class Renderer {
public:
    static std::vector<Color> render(const Scene &scene);
    static std::vector<Color> renderMultithreaded(const Scene &scene, int numThreads = 0);
    static std::vector<Color> renderMultithreadedWithDisplay(const Scene &scene,
                                                             std::shared_ptr<rt::ProgressiveDisplay> display,
                                                             int numThreads = 0);

private:
    struct AAContext
    {
        std::mt19937 &gen;
        std::uniform_real_distribution<double> &dis;
        int samplesPerAxis;
    };

    static Color computePixelColor(const Ray &ray,
                                   const BVH &bvh,
                                   const std::vector<std::unique_ptr<ILight>> &lights,
                                   int depth);

    static Color traceRay(const Ray &ray,
                          const BVH &bvh,
                          const std::vector<std::unique_ptr<ILight>> &lights,
                          int depth);

    static void renderTileWorker(const Scene &scene,
                                 const BVH &bvh,
                                 std::vector<Color> &pixels,
                                 class TileScheduler &scheduler);

    static void renderTileWorkerWithDisplay(const Scene &scene,
                                            const BVH &bvh,
                                            std::vector<Color> &pixels,
                                            class TileScheduler &scheduler,
                                            std::shared_ptr<rt::ProgressiveDisplay> display);

    static Color renderPixelWithAA(const Scene &scene, const BVH &bvh,
                                   int i, int j, AAContext &aa);

    static Color computeTilePixelColor(const Scene &scene, const BVH &bvh,
                                       int i, int j, AAContext &aa);

    static Color computeDirectLighting(const Ray &ray, const HitRecord &hit,
                                       const BVH &bvh,
                                       const std::vector<std::unique_ptr<ILight>> &lights,
                                       const Color &diffuse);

    static bool castShadowRayThrough(const BVH &bvh, const HitRecord &hit,
                                     const Vec3 &lightDir, double maxDistance);

    static Color computeSingleLightColor(const Ray &ray, const HitRecord &hit,
                                         const std::unique_ptr<ILight> &light,
                                         const Vec3 &lightDir, const Color &diffuse);

    static Color computeTransparencyColor(const Ray &ray, const HitRecord &hit,
                                          const BVH &bvh,
                                          const std::vector<std::unique_ptr<ILight>> &lights,
                                          int depth);

    static Color applyReflection(const Ray &ray, const HitRecord &hit,
                                 const BVH &bvh,
                                 const std::vector<std::unique_ptr<ILight>> &lights,
                                 int depth, const Color &base);
};

#endif // RT_RENDERER_HPP
