/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** Utils
*/

#include "rt/core/Utils.hpp"
#include <fstream>

bool Utils::fileExists(const std::string &path)
{
    std::ifstream file(path);
    return file.good();
}
