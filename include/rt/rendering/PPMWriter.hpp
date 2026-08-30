/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** PPMWriter
*/

#ifndef RT_PPMWRITER_HPP
    #define RT_PPMWRITER_HPP

    #include <string>
    #include <vector>
    #include "rt/math/Color.hpp"

class PPMWriter {
public:
    static bool write(const std::string &filename, 
                     const std::vector<Color> &pixels, 
                     int width, int height);

private:
    static int toInt(double value);
};

#endif // RT_PPMWRITER_HPP
