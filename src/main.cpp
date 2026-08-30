/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** main
*/

#include <iostream>
#include <memory>
#include <vector>
#include "rt/scene/Scene.hpp"
#include "rt/scene/LibconfigLoader.hpp"
#include "rt/rendering/Renderer.hpp"
#include "rt/rendering/PathTracer.hpp"
#include "rt/rendering/PPMWriter.hpp"
#include "rt/core/Utils.hpp"
#include "rt/core/PluginManager.hpp"
#include "rt/core/Factory.hpp"
#include "rt/math/Color.hpp"
#include "app/RenderConfig.hpp"
#include "app/AppDenoiser.hpp"
#include "app/InteractiveMode.hpp"

class ApplicationCleanup
{
public:
    ~ApplicationCleanup()
    {
        Factory::clearAllRegistries();
    }
};

static int renderOffscreen(const Scene &scene, const RenderConfig &cfg,
    std::vector<Color> &pixels, DenoiserPtr &denoiser)
{
    if (cfg.usePathTracing)
        pixels = PathTracer::render(scene, cfg.samplesPerPixel);
    else if (cfg.singleThread)
        pixels = Renderer::render(scene);
    else
        pixels = Renderer::renderMultithreaded(scene);
    if (denoiser && cfg.useDenoise)
    {
        std::cout << "Denoising..." << std::endl;
        pixels = denoiser->denoise(pixels, scene.width, scene.height);
        std::cout << "Denoising complete!" << std::endl;
    }
    if (!PPMWriter::write(cfg.outputFile, pixels, scene.width, scene.height))
    {
        std::cerr << "Error: failed to write output image" << std::endl;
        return 84;
    }
    std::cout << "Image generated: " << cfg.outputFile
              << " (" << scene.width << "x" << scene.height << ")" << std::endl;
    return 0;
}

static void loadAndListPlugins()
{
    auto &pluginMgr = PluginManager::instance();
    pluginMgr.loadPluginsFromDirectory("./plugins");
    std::cout << "Registered primitives:" << std::endl;
    for (const auto &name : PrimitiveRegistry::instance().getRegisteredTypes())
        std::cout << "  - " << name << std::endl;
}

static std::unique_ptr<Scene> loadScene(const std::string &path, LibconfigLoader &loader)
{
    std::unique_ptr<Scene> scene;
    try
    {
        scene = loader.loadScene(path);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error loading scene: " << e.what() << std::endl;
    }
    return scene;
}

int main(int ac, char *av[])
{
    ApplicationCleanup cleanup;
    loadAndListPlugins();
    RenderConfig cfg;
    if (!parseArgs(ac, av, cfg))
        return 84;
    if (!Utils::fileExists(cfg.sceneFile))
    {
        std::cerr << "Error: scene file '" << cfg.sceneFile << "' not found" << std::endl;
        return 84;
    }
    LibconfigLoader loader;
    std::unique_ptr<Scene> scene = loadScene(cfg.sceneFile, loader);
    if (!scene)
        return 84;
    std::vector<Color> pixels;
    DenoiserPtr denoiser = makeNullDenoiser();
    if (cfg.useDenoise)
        denoiser = loadDenoiser();
    if (cfg.singleThread || cfg.noDisplay)
        return renderOffscreen(*scene, cfg, pixels, denoiser);
    return runInteractiveMode(cfg, scene, pixels, denoiser, loader);
}
