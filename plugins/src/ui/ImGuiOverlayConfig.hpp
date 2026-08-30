/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** libconfig field helpers for ImGuiOverlay
*/

#pragma once
#include <libconfig.h++>

/** @brief Sets or creates a float field in parent. */
void cfgSetFloat(libconfig::Setting &parent, const char *key, double val);

/** @brief Sets or creates a string field in parent. */
void cfgSetString(libconfig::Setting &parent, const char *key, const char *val);

/** @brief Sets or creates a 3-element float array field in parent. */
void cfgSetColorArr(libconfig::Setting &parent, const char *key, const float c[3]);

/** @brief Reads a float field from parent, returning def when absent. */
float cfgGetFloat(libconfig::Setting &parent, const char *key, float def = 0.f);

/** @brief Reads a 3-element float array from parent ignto out. */
void cfgGetColor(
    libconfig::Setting &parent, const char *key,
    float out[3], float def = 0.f);

/** @brief Appends a new 3-element float array field to parent. */
void cfgAddVec3(libconfig::Setting &parent, const char *key, const float v[3]);
