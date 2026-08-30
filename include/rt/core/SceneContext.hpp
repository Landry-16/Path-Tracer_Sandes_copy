/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** SceneContext
*/

#ifndef RT_SCENECONTEXT_HPP
    #define RT_SCENECONTEXT_HPP

    #include <functional>
    #include "rt/scene/Scene.hpp"

/**
 * @brief Singleton providing cross-module access to the active scene,
 * its file path, and notification callbacks.
 */
class SceneContext {
public:
    /** @brief Returns the unique instance of SceneContext. */
    static SceneContext &instance()
    {
        static SceneContext ctx;
        return ctx;
    }

    /** @brief Sets the active scene pointer. @param scene: the new scene */
    void setScene(Scene *scene) { scene_ = scene; }
    /** @brief Returns a read-only pointer to the active scene. */
    const Scene *getScene() const { return scene_; }
    /** @brief Returns a mutable pointer to the active scene. */
    Scene *getMutableScene() { return scene_; }
    /** @brief Returns true if a scene is currently loaded. */
    bool hasScene() const { return scene_ != nullptr; }

    /** @brief Stores the path of the current scene configuration file. */
    void setScenePath(const std::string &p) { scenePath_ = p; }
    /** @brief Returns the path of the current scene configuration file. */
    const std::string &getScenePath() const { return scenePath_; }

    /** @brief Registers the callback invoked when a re-render is requested. */
    void setRerenderCallback(std::function<void()> cb)
    {
        rerenderCb_ = std::move(cb);
    }
    /** @brief Registers the callback invoked when a scene reload is requested. */
    void setReloadCallback(std::function<void()> cb)
    {
        reloadCb_ = std::move(cb);
    }
    /** @brief Registers the callback invoked after the scene has been reloaded. */
    void setSceneChangedCallback(std::function<void()> cb)
    {
        sceneChangedCb_ = std::move(cb);
    }

    /** @brief Fires the re-render callback if it is registered. */
    void triggerRerender()
    {
        if (rerenderCb_)
            rerenderCb_();
    }
    /** @brief Fires the reload callback if it is registered. */
    void triggerReload()
    {
        if (reloadCb_)
            reloadCb_();
    }
    /** @brief Fires the scene-changed callback if it is registered. */
    void notifySceneChanged()
    {
        if (sceneChangedCb_)
            sceneChangedCb_();
    }

private:
    SceneContext() = default;
    SceneContext(const SceneContext &) = delete;
    SceneContext &operator=(const SceneContext &) = delete;

    Scene *scene_ = nullptr;
    std::string scenePath_;
    std::function<void()> rerenderCb_;
    std::function<void()> reloadCb_;
    std::function<void()> sceneChangedCb_;
};

#endif // RT_SCENECONTEXT_HPP
