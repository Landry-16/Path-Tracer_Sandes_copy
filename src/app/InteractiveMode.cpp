/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** InteractiveMode
*/

#include "InteractiveMode.hpp"
#include "rt/scene/Camera.hpp"
#include "rt/rendering/Renderer.hpp"
#include "rt/rendering/PathTracer.hpp"
#include "rt/rendering/PPMWriter.hpp"
#include "rt/display/ProgressiveDisplay.hpp"
#include "rt/core/FileWatcher.hpp"
#include "rt/core/SceneContext.hpp"
#include "rt/math/Vec3.hpp"
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <map>

using CamAction = std::function<void(Camera&, double, double)>;

static const std::map<std::string, CamAction> s_camActions = {
    {"forward", [](Camera &c, double m, double) { c.moveForward(m); }},
    {"backward", [](Camera &c, double m, double) { c.moveBackward(m); }},
    {"left", [](Camera &c, double m, double) { c.moveLeft(m); }},
    {"right", [](Camera &c, double m, double) { c.moveRight(m); }},
    {"up", [](Camera &c, double m, double) { c.moveUp(m); }},
    {"down", [](Camera &c, double m, double) { c.moveDown(m); }},
    {"pitch_up", [](Camera &c, double, double r) { c.rotatePitch(r); }},
    {"pitch_down", [](Camera &c, double, double r) { c.rotatePitch(-r); }},
    {"yaw_left", [](Camera &c, double, double r) { c.rotateYaw(-r); }},
    {"yaw_right", [](Camera &c, double, double r) { c.rotateYaw(r); }},
};

/**
 * @brief Bundles all mutable state passed between interactive render helpers.
 */
struct InteractiveCtx
{
    std::shared_ptr<rt::ProgressiveDisplay> display;
    rt::FileWatcher &fileWatcher;
    std::unique_ptr<Scene> &scene;
    std::vector<Color> &pixels;
    std::mutex &pixelsMutex;
    std::mutex &cameraMutex;
    std::atomic<bool> &renderComplete;
    std::atomic<bool> &shouldReload;
    std::atomic<bool> &cameraChanged;
    const RenderConfig &cfg;
    DenoiserPtr &denoiser;
    LibconfigLoader &loader;
};

/**
 * @brief Applies a named camera action and updates the display status bar.
 * @param scene: the scene whose camera is modified
 * @param action: key name matching an entry in s_camActions
 * @param moveSpeed: translation speed in scene units
 * @param rotateSpeed: rotation speed in degrees
 * @param display: display used to show the new camera position
 */
static void applyCameraControl(std::unique_ptr<Scene> &scene, const std::string &action,
    double moveSpeed, double rotateSpeed, rt::ProgressiveDisplay &display)
{
    auto it = s_camActions.find(action);
    if (it != s_camActions.end())
        it->second(*scene->camera, moveSpeed, rotateSpeed);
    Vec3 pos = scene->camera->getPosition();
    display.setStatusText("Camera: (" + std::to_string(pos.x) + ", "
        + std::to_string(pos.y) + ", " + std::to_string(pos.z) + ")");
}

/**
 * @brief Renders a single frame into ctx.pixels, running the denoiser if configured.
 * @param ctx: the shared interactive context
 */
static void runRenderFrame(InteractiveCtx &ctx)
{
    std::lock_guard<std::mutex> lock(ctx.pixelsMutex);
    if (ctx.cfg.usePathTracing)
        ctx.pixels = PathTracer::renderWithDisplay(*ctx.scene, ctx.display, ctx.cfg.samplesPerPixel);
    else
        ctx.pixels = Renderer::renderMultithreadedWithDisplay(*ctx.scene, ctx.display);
    if (ctx.denoiser && ctx.cfg.useDenoise && !ctx.pixels.empty())
    {
        ctx.display->setStatusText("Denoising...");
        ctx.pixels = ctx.denoiser->denoise(ctx.pixels, ctx.scene->width, ctx.scene->height);
        ctx.display->updateRegion(0, 0, ctx.scene->width, ctx.scene->height, ctx.pixels);
        ctx.display->setStatusText("Denoising complete!");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

/**
 * @brief Blocks until a reload, camera change, or file-watcher event occurs.
 * @param ctx: the shared interactive context
 */
static void waitForRenderTrigger(InteractiveCtx &ctx)
{
    while (ctx.display->isOpen() && !ctx.shouldReload.load() && !ctx.cameraChanged.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (ctx.fileWatcher.hasChanged())
        {
            std::cout << "Scene file changed, reloading..." << std::endl;
            ctx.shouldReload.store(true);
        }
    }
}

/**
 * @brief Handles a pending camera change or scene reload after each frame.
 * @param ctx: the shared interactive context
 */
static void processRenderTrigger(InteractiveCtx &ctx)
{
    if (ctx.cameraChanged.load())
    {
        ctx.cameraChanged.store(false);
        ctx.renderComplete.store(false);
        std::cout << "Camera changed, rerendering..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return;
    }
    if (!ctx.shouldReload.load())
        return;
    ctx.shouldReload.store(false);
    ctx.renderComplete.store(false);
    try
    {
        SceneContext::instance().setScene(nullptr);
        ctx.scene = ctx.loader.loadScene(ctx.cfg.sceneFile);
        SceneContext::instance().setScene(ctx.scene.get());
        ctx.fileWatcher.reset();
        SceneContext::instance().notifySceneChanged();
        ctx.display->setStatusText("Scene reloaded!");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error reloading scene: " << e.what() << std::endl;
        ctx.display->setStatusText("Error: Failed to reload scene!");
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

/**
 * @brief Runs the render, wait, and reload cycle until the display is closed.
 * Clears all SceneContext callbacks on exit.
 * @param ctx: the shared interactive context
 */
static void runRenderLoop(InteractiveCtx &ctx)
{
    while (ctx.display->isOpen())
    {
        SceneContext::instance().setScene(ctx.scene.get());
        ctx.display->setStatusText("Rendering...");
        runRenderFrame(ctx);
        if (!ctx.display->isOpen())
            break;
        ctx.renderComplete.store(true);
        ctx.display->setStatusText("Render complete : arrows/IJKL: camera, S: save, R: reload");
        waitForRenderTrigger(ctx);
        processRenderTrigger(ctx);
    }
    SceneContext::instance().setScene(nullptr);
    SceneContext::instance().setRerenderCallback(nullptr);
    SceneContext::instance().setReloadCallback(nullptr);
    SceneContext::instance().setSceneChangedCallback(nullptr);
}

/**
 * @brief Wires save, reload, camera, and SceneContext callbacks for interactive mode.
 * @param ctx: the shared interactive context
 */
static void setupInteractiveCallbacks(InteractiveCtx &ctx)
{
    ctx.display->setSaveCallback([&ctx]()
    {
        std::lock_guard<std::mutex> lock(ctx.pixelsMutex);
        if (!ctx.pixels.empty())
        {
            bool ok = PPMWriter::write(ctx.cfg.outputFile, ctx.pixels,
                ctx.scene->width, ctx.scene->height);
            ctx.display->setStatusText(ok ? "Saved: " + ctx.cfg.outputFile : "Error: Failed to save!");
        }
    });
    ctx.display->setReloadCallback([&ctx]() { ctx.shouldReload.store(true); });
    ctx.display->setCameraControlCallback([&ctx](const std::string &action)
    {
        std::lock_guard<std::mutex> lock(ctx.cameraMutex);
        applyCameraControl(ctx.scene, action, 0.3, 5.0, *ctx.display);
        ctx.cameraChanged.store(true);
    });
    SceneContext::instance().setRerenderCallback([&ctx]()
    {
        std::lock_guard<std::mutex> lock(ctx.cameraMutex);
        ctx.cameraChanged.store(true);
    });
    SceneContext::instance().setScenePath(ctx.cfg.sceneFile);
    SceneContext::instance().setReloadCallback([&ctx]()
    {
        ctx.shouldReload.store(true);
    });
}

int runInteractiveMode(const RenderConfig &cfg, std::unique_ptr<Scene> &scene,
    std::vector<Color> &pixels, DenoiserPtr &denoiser, LibconfigLoader &loader)
{
    auto display = std::make_shared<rt::ProgressiveDisplay>(scene->width, scene->height);
    rt::FileWatcher fileWatcher(cfg.sceneFile);
    std::atomic<bool> renderComplete{false}, shouldReload{false}, cameraChanged{false};
    std::mutex pixelsMutex, cameraMutex;
    InteractiveCtx ctx{display, fileWatcher, scene, pixels, pixelsMutex, cameraMutex,
        renderComplete, shouldReload, cameraChanged, cfg, denoiser, loader};
    setupInteractiveCallbacks(ctx);
    std::thread renderThread([&ctx]() { runRenderLoop(ctx); });
    display->run();
    renderThread.join();
    if (renderComplete.load() && !pixels.empty())
        if (PPMWriter::write(cfg.outputFile, pixels, scene->width, scene->height))
            std::cout << "Image generated: " << cfg.outputFile << std::endl;
    return 0;
}
