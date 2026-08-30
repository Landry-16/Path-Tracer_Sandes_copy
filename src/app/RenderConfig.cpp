/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** RenderConfig
*/

#include "RenderConfig.hpp"
#include <iostream>
#include <map>
#include <algorithm>
#include <cstdlib>

bool parseArgs(int ac, char *av[], RenderConfig &cfg)
{
    if (ac < 2)
    {
        std::cerr << "Usage: " << av[0]
                  << " <scene_file> [output_file] [--single-thread] [--no-display]"
                     " [--path-tracing] [--samples N] [--denoise]" << std::endl;
        return false;
    }
    cfg.sceneFile = av[1];
    static const std::map<std::string, bool RenderConfig::*> s_flags = {
        {"--single-thread", &RenderConfig::singleThread},
        {"--no-display",    &RenderConfig::noDisplay},
        {"--path-tracing",  &RenderConfig::usePathTracing},
        {"--denoise",       &RenderConfig::useDenoise},
    };
    for (int i = 2; i < ac; ++i)
    {
        std::string arg(av[i]);
        auto it = s_flags.find(arg);
        if (it != s_flags.end())
            cfg.*(it->second) = true;
        else if (arg == "--samples" && i + 1 < ac)
            cfg.samplesPerPixel = std::max(1, std::atoi(av[++i]));
        else if (arg.find("--") != 0)
            cfg.outputFile = arg;
    }
    return true;
}
