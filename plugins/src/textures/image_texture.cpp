/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** image_texture
*/

#define STB_IMAGE_IMPLEMENTATION
    #include "stb_image.h"
    #include <memory>
    #include <iostream>
    #include <algorithm>
    #include "rt/interfaces/ITexture.hpp"
    #include "rt/core/Factory.hpp"
    #include "rt/math/Color.hpp"

class ImageTexture : public ITexture {
public:
    ImageTexture(const std::string &filename, double scale = 1.0)
        : data(nullptr), width(0), height(0), channels(0), scale(scale)
    {
        data = stbi_load(filename.c_str(), &width, &height, &channels, 3);

        if (!data) {
            std::cerr << "Failed to load texture: " << filename
                      << " (" << stbi_failure_reason() << ")" << std::endl;
            width = 1;
            height = 1;
        }
    }

    ~ImageTexture()
    {
        if (data) {
            stbi_image_free(data);
        }
    }

    Color sample(double u, double v) const override
    {
        if (!data) {
            return Color(1.0, 0.0, 1.0);
        }
        u = (u * scale) - std::floor(u * scale);
        v = (v * scale) - std::floor(v * scale);
        int x = static_cast<int>(u * width);
        int y = static_cast<int>((1.0 - v) * height);
        x = std::max(0, std::min(width - 1, x));
        y = std::max(0, std::min(height - 1, y));
        int index = (y * width + x) * 3;
        double r = data[index] / 255.0;
        double g = data[index + 1] / 255.0;
        double b = data[index + 2] / 255.0;
        return Color(r, g, b);
    }

private:
    unsigned char *data;
    int width;
    int height;
    int channels;
    double scale;
};

extern "C" {
    void rt_plugin_register()
    {
        TextureRegistry::instance().registerType("image",
            [](const TextureParams &p) -> std::unique_ptr<ITexture> {
                return std::make_unique<ImageTexture>(p.imagePath, p.scale);
            }
        );
    }
}
