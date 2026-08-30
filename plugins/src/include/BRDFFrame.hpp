/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** Pre-computed BRDF evaluation frame
*/

#ifndef RT_PLUGINS_BRDF_FRAME_HPP
#define RT_PLUGINS_BRDF_FRAME_HPP

#include "rt/math/Vec3.hpp"
#include <algorithm>

// Holds all pre-computed vectors and dot products needed for one BRDF evaluation.
// Built once from and passed to all sub-functions.
struct BRDFFrame
{
    Vec3 N;
    Vec3 V;
    Vec3 L;
    Vec3 H;
    double NdotV;
    double NdotL;

    static BRDFFrame make(const Vec3 &wo, const Vec3 &wi, const Vec3 &normal)
    {
        BRDFFrame f;
        f.N = normal.normalized();
        f.V = wo.normalized();
        f.L = wi.normalized();
        f.H = (f.V + f.L).normalized();
        f.NdotV = std::max(f.N.dot(f.V), 0.0);
        f.NdotL = std::max(f.N.dot(f.L), 0.0);
        return f;
    }
};

#endif
