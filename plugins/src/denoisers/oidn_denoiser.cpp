/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** oidn_denoiser
*/

#include "rt/interfaces/IDenoiser.hpp"
#include "rt/math/Color.hpp"
#include <OpenImageDenoise/oidn.hpp>
#include <vector>
#include <iostream>
#include <cstring>

// Groups all OIDN filter buffers and dimensions into one struct
struct FilterBuffers
{
    std::vector<float> input;
    std::vector<float> normals;
    std::vector<float> albedo;
    std::vector<float> output;
    int width;
    int height;
};

class OIDNDenoiser : public IDenoiser {
private:
    oidn::DeviceRef device;
    bool available;

    static std::vector<float> colorsToFloat(const std::vector<Color> &c)
    {
        std::vector<float> buf(c.size() * 3);
        for (size_t i = 0; i < c.size(); ++i) {
            buf[i*3+0] = static_cast<float>(c[i].x);
            buf[i*3+1] = static_cast<float>(c[i].y);
            buf[i*3+2] = static_cast<float>(c[i].z);
        }
        return buf;
    }

    static std::vector<Color> floatToColors(const std::vector<float> &buf, int n)
    {
        std::vector<Color> result(n);
        for (int i = 0; i < n; ++i)
            result[i] = Color(buf[i*3+0], buf[i*3+1], buf[i*3+2]);
        return result;
    }

    bool runFilter(FilterBuffers &buf)
    {
        try {
            oidn::FilterRef filter = device.newFilter("RT");
            filter.setImage("color", buf.input.data(), oidn::Format::Float3, buf.width, buf.height);
            filter.setImage("output", buf.output.data(), oidn::Format::Float3, buf.width, buf.height);
            if (!buf.normals.empty()) filter.setImage("normal", buf.normals.data(), oidn::Format::Float3, buf.width, buf.height);
            if (!buf.albedo.empty()) filter.setImage("albedo", buf.albedo.data(), oidn::Format::Float3, buf.width, buf.height);
            filter.set("hdr", true);
            filter.commit();
            filter.execute();
            const char *err;
            if (device.getError(err) != oidn::Error::None) {
                std::cerr << "OIDN error: " << err << std::endl; return false;
            }
            return true;
        } catch (const std::exception &e) {
            std::cerr << "OIDN exception: " << e.what() << std::endl; return false;
        }
    }

public:
    OIDNDenoiser() : available(false)
    {
        try {
            device = oidn::newDevice();
            device.commit();

            const char *errorMessage;
            if (device.getError(errorMessage) == oidn::Error::None) {
                available = true;
                std::cout << "OIDN Denoiser initialized successfully" << std::endl;
            } else {
                std::cerr << "OIDN Error: " << errorMessage << std::endl;
            }
        } catch (const std::exception &e) {
            std::cerr << "Failed to initialize OIDN: " << e.what() << std::endl;
            available = false;
        }
    }

    ~OIDNDenoiser() override = default;

    std::vector<Color> denoise(
        const std::vector<Color> &noisyImage, int width, int height
    ) override
    {
        return denoiseWithAuxiliary(noisyImage, {}, {}, width, height);
    }

    std::vector<Color> denoiseWithAuxiliary(
        const std::vector<Color> &noisyImage,
        const std::vector<Color> &normals,
        const std::vector<Color> &albedo,
        int width, int height
    ) override
    {
        if (!available) {
            std::cerr << "OIDN not available, returning original image" << std::endl;
            return noisyImage;
        }
        FilterBuffers buf;
        buf.input = colorsToFloat(noisyImage);
        buf.normals = normals.empty() ? std::vector<float>{} : colorsToFloat(normals);
        buf.albedo = albedo.empty() ? std::vector<float>{} : colorsToFloat(albedo);
        buf.output = std::vector<float>(width * height * 3);
        buf.width = width;
        buf.height = height;
        if (!runFilter(buf)) return noisyImage;
        return floatToColors(buf.output, width * height);
    }

    std::string getName() const override
    {
        return "Intel Open Image Denoise";
    }

    bool isAvailable() const override
    {
        return available;
    }
};

extern "C" {
    IDenoiser *create_denoiser()
    {
        return new OIDNDenoiser();
    }

    void destroy_denoiser(IDenoiser *denoiser)
    {
        delete denoiser;
    }
}
