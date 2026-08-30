/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** AppDenoiser
*/

#pragma once
#include <memory>
#include <functional>
#include "rt/interfaces/IDenoiser.hpp"

using DenoiserPtr = std::unique_ptr<IDenoiser, std::function<void(IDenoiser*)>>;

DenoiserPtr makeNullDenoiser();
DenoiserPtr loadDenoiser();
