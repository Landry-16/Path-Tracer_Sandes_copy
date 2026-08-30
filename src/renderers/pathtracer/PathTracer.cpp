/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** PathTracer : render orchestration and tile workers
*/

#include "rt/rendering/PathTracer.hpp"
#include "rt/rendering/TileScheduler.hpp"
#include "rt/display/ProgressiveDisplay.hpp"
#include <iostream>
#include <chrono>
#include <thread>

static BVH buildAndLogBVH(const Scene &scene)
{
    std::cout << "Building BVH for " << scene.primitives.size() << " primitives..." << std::endl;
    auto t0 = std::chrono::high_resolution_clock::now();
    BVH bvh(scene.primitives);
    std::cout << "BVH built in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::high_resolution_clock::now() - t0).count()
              << "ms" << std::endl;
    return bvh;
}

static int getThreadCount()
{
    int n = static_cast<int>(std::thread::hardware_concurrency());
    return n > 0 ? n : 4;
}

static Color applyToneMapping(const Color &c)
{
    return Color(std::sqrt(c.x / (1.0 + c.x)),
                 std::sqrt(c.y / (1.0 + c.y)),
                 std::sqrt(c.z / (1.0 + c.z)));
}

Color PathTracer::samplePixel(const Scene &scene, const BVH &bvh,
    int i, int j, SampleCtx &ctx)
{
    Color accumulated(0, 0, 0);
    for (int s = 0; s < ctx.samplesPerPixel; ++s)
    {
        double u = (i + ctx.dist(ctx.rng)) / (scene.width - 1.0);
        double v = (j + ctx.dist(ctx.rng)) / (scene.height - 1.0);
        Ray ray = scene.camera->generateRay(
            static_cast<int>(u * scene.width),
            scene.height - 1 - static_cast<int>(v * scene.height));
        accumulated += trace(ray, {bvh, scene.lights, 0, ctx.rng});
    }
    return applyToneMapping(accumulated * (1.0 / ctx.samplesPerPixel));
}

std::vector<Color> PathTracer::render(const Scene &scene, int samplesPerPixel)
{
    return renderWithDisplay(scene, nullptr, samplesPerPixel);
}

void PathTracer::renderTileWorker(const Scene &scene, const BVH &bvh,
    std::vector<Color> &pixels, TileScheduler &scheduler,
    int samplesPerPixel, std::shared_ptr<rt::ProgressiveDisplay> display)
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    SampleCtx ctx{rng, dist, samplesPerPixel};
    Tile tile;
    while (scheduler.getNextTile(tile) && (!display || display->isOpen()))
    {
        std::vector<Color> tilePixels;
        tilePixels.reserve((tile.endY - tile.startY) * (tile.endX - tile.startX));
        for (int j = tile.startY; j < tile.endY; ++j)
            for (int i = tile.startX; i < tile.endX; ++i)
            {
                Color c = samplePixel(scene, bvh, i, j, ctx);
                pixels[j * scene.width + i] = c;
                tilePixels.push_back(c);
            }
        if (display)
            display->updateRegion(tile.startX, tile.startY,
                tile.endX - tile.startX, tile.endY - tile.startY, tilePixels);
    }
}

std::vector<Color> PathTracer::renderWithDisplay(const Scene &scene,
    std::shared_ptr<rt::ProgressiveDisplay> display, int samplesPerPixel)
{
    int numThreads = getThreadCount();
    BVH bvh = buildAndLogBVH(scene);
    std::vector<Color> pixels(scene.width * scene.height, Color(0, 0, 0));
    TileScheduler scheduler(scene.width, scene.height, 64);
    std::cout << "Path tracing with " << numThreads << " threads ("
              << scheduler.totalTiles() << " tiles, "
              << samplesPerPixel << " samples/pixel)..." << std::endl;
    std::cout << "Scene has " << scene.lights.size() << " lights" << std::endl;
    auto t0 = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> threads;
    threads.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i)
        threads.emplace_back(renderTileWorker, std::cref(scene), std::cref(bvh),
            std::ref(pixels), std::ref(scheduler), samplesPerPixel, display);
    for (auto &t : threads)
        t.join();
    std::cout << "Path traced in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::high_resolution_clock::now() - t0).count()
              << "ms" << std::endl;
    return pixels;
}
