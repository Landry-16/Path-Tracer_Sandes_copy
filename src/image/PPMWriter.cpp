/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** PPMWriter
*/

#include "rt/rendering/PPMWriter.hpp"
#include <fstream>
#include <algorithm>

bool PPMWriter::write(const std::string &filename,
                     const std::vector<Color> &pixels,
                     int width, int height)
{
    std::ofstream file(filename);
    if (!file.is_open())
        return false;

    file << "P3\n" << width << " " << height << "\n255\n";

    for (int i = 0; i < width * height; ++i) {
        file << toInt(pixels[i].x) << " "
             << toInt(pixels[i].y) << " "
             << toInt(pixels[i].z) << "\n";
    }

    file.close();
    return true;
}

int PPMWriter::toInt(double value)
{
    int result = static_cast<int>(value * 255.99);
    return std::max(0, std::min(255, result));
}
