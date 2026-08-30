/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** InteractiveMode
*/

#pragma once
#include <memory>
#include <vector>
#include "rt/scene/Scene.hpp"
#include "rt/math/Color.hpp"
#include "rt/scene/LibconfigLoader.hpp"
#include "AppDenoiser.hpp"
#include "RenderConfig.hpp"

int runInteractiveMode(const RenderConfig &cfg, std::unique_ptr<Scene> &scene,
    std::vector<Color> &pixels, DenoiserPtr &denoiser, LibconfigLoader &loader);
