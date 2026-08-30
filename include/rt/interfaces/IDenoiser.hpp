/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** IDenoiser
*/

#ifndef RT_IDENOISER_HPP
    #define RT_IDENOISER_HPP

    #include "rt/math/Color.hpp"
    #include <vector>
    #include <string>

static constexpr std::string_view ODIN_DENOISER_LIB_PATH = "./plugins/raytracer_oidn_denoiser.so";
static constexpr std::string_view SIMPLE_DENOISER_LIB_PATH = "./plugins/raytracer_simple_denoiser.so";

class IDenoiser {
public:
    virtual ~IDenoiser() = default;
    
    virtual std::vector<Color> denoise(
        const std::vector<Color> &noisyImage,
        int width, int height
    ) = 0;
    
    virtual std::vector<Color> denoiseWithAuxiliary(
        const std::vector<Color> &noisyImage,
        const std::vector<Color> &normals,
        const std::vector<Color> &albedo,
        int width, int height
    ) = 0;
    
    virtual std::string getName() const = 0;
    virtual bool isAvailable() const = 0;
};

#endif // RT_IDENOISER_HPP
