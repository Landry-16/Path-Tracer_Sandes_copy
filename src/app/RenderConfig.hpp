/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** RenderConfig
*/

#pragma once
#include <string>

struct RenderConfig
{
    std::string sceneFile;
    std::string outputFile = "output.ppm";
    bool singleThread = false;
    bool noDisplay = false;
    bool usePathTracing = false;
    bool useDenoise = false;
    int samplesPerPixel = 100;
};

bool parseArgs(int ac, char *av[], RenderConfig &cfg);
