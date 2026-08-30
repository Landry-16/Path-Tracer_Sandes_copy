/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** ISceneLoader
*/

#ifndef RT_ISCENELOADER_HPP
    #define RT_ISCENELOADER_HPP

    #include <string>
    #include <memory>
    #include "rt/scene/Scene.hpp"

class ISceneLoader {
public:
    virtual ~ISceneLoader() = default;
    virtual std::unique_ptr<Scene> loadScene(const std::string &filename) = 0;
};

#endif // RT_ISCENELOADER_HPP
