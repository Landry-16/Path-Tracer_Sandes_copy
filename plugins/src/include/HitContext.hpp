/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** Shared accumulator struct for closest-hit tracking
*/

#ifndef RT_PLUGINS_HIT_CONTEXT_HPP
#define RT_PLUGINS_HIT_CONTEXT_HPP

#include "rt/math/Vec3.hpp"

// Accumulates the closest intersection found during primitive intersection tests.
struct HitContext
{
    double closestT;
    Vec3 normal;
    Vec3 point;
    bool hit;

    explicit HitContext(double maxT) : closestT(maxT), normal(), point(), hit(false)
    {}
};

#endif
