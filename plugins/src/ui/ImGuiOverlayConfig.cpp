/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** libconfig field helpers for ImGuiOverlay
*/

#include "ImGuiOverlayConfig.hpp"

/** @brief Sets a Setting to val, converting to int/long long when needed. */
static void cfgSetFloatVal(libconfig::Setting &s, double val)
{
    switch (s.getType()) {
        case libconfig::Setting::TypeInt: s = (int)val;
            break;
        case libconfig::Setting::TypeInt64: s = (long long)val;
            break;
        default: s = val;
            break;
    }
}

void cfgSetFloat(libconfig::Setting &parent, const char *key, double val)
{
    if (parent.exists(key))
        cfgSetFloatVal(parent[key], val);
    else
        parent.add(key, libconfig::Setting::TypeFloat) = val;
}

void cfgSetString(libconfig::Setting &parent, const char *key, const char *val)
{
    if (parent.exists(key))
        parent[key] = val;
    else
        parent.add(key, libconfig::Setting::TypeString) = val;
}

void cfgSetColorArr(libconfig::Setting &parent, const char *key, const float c[3])
{
    if (parent.exists(key)) {
        parent[key][0] = (double)c[0];
        parent[key][1] = (double)c[1];
        parent[key][2] = (double)c[2];
        return;
    }
    auto &arr = parent.add(key, libconfig::Setting::TypeArray);
    arr.add(libconfig::Setting::TypeFloat) = (double)c[0];
    arr.add(libconfig::Setting::TypeFloat) = (double)c[1];
    arr.add(libconfig::Setting::TypeFloat) = (double)c[2];
}

float cfgGetFloat(libconfig::Setting &parent, const char *key, float def)
{
    if (!parent.exists(key))
        return def;
    auto &s = parent[key];
    if (s.getType() == libconfig::Setting::TypeInt)
        return (float)(int)s;
    if (s.getType() == libconfig::Setting::TypeInt64)
        return (float)(long long)s;
    return (float)(double)s;
}

void cfgGetColor(
    libconfig::Setting &parent, const char *key,
    float out[3], float def)
{
    out[0] = out[1] = out[2] = def;
    if (!parent.exists(key))
        return;
    auto &arr = parent[key];
    if (arr.getLength() < 3)
        return;
    out[0] = (float)(double)arr[0];
    out[1] = (float)(double)arr[1];
    out[2] = (float)(double)arr[2];
}

void cfgAddVec3(libconfig::Setting &parent, const char *key, const float v[3])
{
    auto &arr = parent.add(key, libconfig::Setting::TypeArray);
    arr.add(libconfig::Setting::TypeFloat) = (double)v[0];
    arr.add(libconfig::Setting::TypeFloat) = (double)v[1];
    arr.add(libconfig::Setting::TypeFloat) = (double)v[2];
}
