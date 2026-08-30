/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** simple_denoiser
*/

#include "rt/interfaces/IDenoiser.hpp"
#include "rt/math/Color.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

class SimpleBilateralDenoiser : public IDenoiser {
private:
    int kernelRadius;
    double spatialSigma;
    double colorSigma;

    double gaussian(double x, double sigma) const
    {
        return std::exp(-(x * x) / (2.0 * sigma * sigma));
    }

    Color getPixel(const std::vector<Color> &image, int x, int y, int width, int height) const
    {
        x = std::clamp(x, 0, width - 1);
        y = std::clamp(y, 0, height - 1);
        return image[y * width + x];
    }

    double colorDistance(const Color &a, const Color &b) const
    {
        double dr = a.x - b.x, dg = a.y - b.y, db = a.z - b.z;
        return std::sqrt(dr * dr + dg * dg + db * db);
    }

    double bilateralWeight(const Color &center, const Color &nbr, int kx, int ky) const
    {
        double spatDist = std::sqrt(static_cast<double>(kx * kx + ky * ky));
        double colorDist = colorDistance(center, nbr);
        return gaussian(spatDist, spatialSigma) * gaussian(colorDist, colorSigma);
    }

    Color filterPixel(const std::vector<Color> &img, int x, int y, int w, int h) const
    {
        Color center = getPixel(img, x, y, w, h);
        Color sum(0, 0, 0);
        double wSum = 0.0;
        for (int ky = -kernelRadius; ky <= kernelRadius; ++ky) {
            for (int kx = -kernelRadius; kx <= kernelRadius; ++kx) {
                if (x+kx < 0 || x+kx >= w || y+ky < 0 || y+ky >= h) continue;
                Color nbr = img[(y + ky) * w + (x + kx)];
                double wt = bilateralWeight(center, nbr, kx, ky);
                sum.x += nbr.x * wt; sum.y += nbr.y * wt; sum.z += nbr.z * wt;
                wSum += wt;
            }
        }
        return (wSum > 0.0001) ? Color(sum.x / wSum, sum.y / wSum, sum.z / wSum) : center;
    }

public:
    SimpleBilateralDenoiser(int radius = 5, double spatialSig = 2.0, double colorSig = 0.3)
        : kernelRadius(radius), spatialSigma(spatialSig), colorSigma(colorSig)
    {}

    ~SimpleBilateralDenoiser() override = default;

    std::vector<Color> denoiseWithAuxiliary(
        const std::vector<Color> &noisyImage,
        const std::vector<Color>&, const std::vector<Color>&,
        int width, int height
    ) override
    {
        return denoise(noisyImage, width, height);
    }

    std::vector<Color> denoise(
        const std::vector<Color> &noisyImage, int width, int height
    ) override
    {
        std::vector<Color> result(width * height);
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                result[y * width + x] = filterPixel(noisyImage, x, y, width, height);
        return result;
    }

    std::string getName() const override
    {
        return "Simple Bilateral Denoiser";
    }

    bool isAvailable() const override
    {
        return true;
    }
};

extern "C" {
    IDenoiser *create_simple_denoiser()
    {
        return new SimpleBilateralDenoiser();
    }

    void destroy_simple_denoiser(IDenoiser *denoiser)
    {
        delete denoiser;
    }
}
