/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** SceneBuilder
*/

#ifndef RT_SCENEBUILDER_HPP
    #define RT_SCENEBUILDER_HPP

    #include <memory>
    #include <vector>
    #include "rt/scene/Scene.hpp"
    #include "rt/core/Factory.hpp"
    #include "rt/scene/Camera.hpp"

class SceneBuilder {
public:
    SceneBuilder() : scene(std::make_unique<Scene>()) {}
    
    SceneBuilder &setResolution(int w, int h) {
        scene->width = w;
        scene->height = h;
        return *this;
    }
    
    SceneBuilder &setCamera(const Vec3 &position, const Vec3 &lookAt, const Vec3 &up, double fov) {
        scene->camera = std::make_unique<Camera>(
            position, lookAt, up, fov, scene->width, scene->height
        );
        return *this;
    }
    
    SceneBuilder &addPrimitive(const std::string &type, const PrimitiveParams &params) {
        auto primitive = Factory::createPrimitive(type, params);
        scene->primitives.push_back(std::move(primitive));
        return *this;
    }
    
    SceneBuilder &addLight(const std::string &type, const LightParams &params) {
        auto light = Factory::createLight(type, params);
        scene->lights.push_back(std::move(light));
        return *this;
    }
    
    SceneBuilder &addMaterial(const std::string &type, const MaterialParams &params) {
        auto material = Factory::createMaterial(type, params);
        scene->materials.push_back(std::move(material));
        return *this;
    }
    
    std::unique_ptr<Scene> build() {
        if (!scene->camera) {
            throw std::runtime_error("Scene must have a camera");
        }
        return std::move(scene);
    }
    
    Scene *getScene() {
        return scene.get();
    }

private:
    std::unique_ptr<Scene> scene;
};

#endif // RT_SCENEBUILDER_HPP
