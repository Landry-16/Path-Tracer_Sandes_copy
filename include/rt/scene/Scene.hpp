/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** Scene
*/

#ifndef RT_SCENE_HPP
    #define RT_SCENE_HPP

    #include <vector>
    #include <memory>
    #include "rt/scene/Camera.hpp"
    #include "rt/interfaces/IPrimitive.hpp"
    #include "rt/interfaces/ILight.hpp"
    #include "rt/interfaces/IMaterial.hpp"

class Scene {
public:
    std::unique_ptr<Camera> camera;
    std::vector<std::unique_ptr<IPrimitive>> primitives;
    std::vector<std::unique_ptr<ILight>> lights;
    std::vector<std::shared_ptr<IMaterial>> materials;

    int width;
    int height;

    int antialiasingSamples = 1;
    double aperture = 0.0;
    double focusDistance = 1.0;
};

#endif // RT_SCENE_HPP
