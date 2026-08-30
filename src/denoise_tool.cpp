/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** denoise_tool
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <dlfcn.h>
#include "rt/interfaces/IDenoiser.hpp"
#include "rt/math/Color.hpp"
#include "rt/rendering/PPMWriter.hpp"

typedef IDenoiser *(*CreateDenoiserFunc)();
typedef void (*DestroyDenoiserFunc)(IDenoiser*);

class DenoiserOwner
{
public:
    DenoiserOwner() = default;
    DenoiserOwner(IDenoiser *ptr, DestroyDenoiserFunc destroy, void *lib)
        : _ptr(ptr), _destroy(destroy), _lib(lib) {}

    ~DenoiserOwner()
    {
        if (_ptr && _destroy)
            _destroy(_ptr);
        if (_lib)
            dlclose(_lib);
    }

    DenoiserOwner(DenoiserOwner&& o) noexcept
        : _ptr(o._ptr), _destroy(o._destroy), _lib(o._lib)
    {
        o._ptr = nullptr;
        o._lib = nullptr;
    }

    DenoiserOwner &operator=(DenoiserOwner&&) = delete;
    DenoiserOwner(const DenoiserOwner&) = delete;
    DenoiserOwner &operator=(const DenoiserOwner&) = delete;

    explicit operator bool() const { return _ptr != nullptr; }
    IDenoiser *get() const { return _ptr; }

private:
    IDenoiser *_ptr = nullptr;
    DestroyDenoiserFunc _destroy = nullptr;
    void *_lib = nullptr;
};

static std::vector<Color> loadPPM(const std::string &filename, int &width, int &height)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open " << filename << std::endl;
        return {};
    }
    std::string format;
    file >> format;
    if (format != "P3")
    {
        std::cerr << "Only P3 PPM format supported" << std::endl;
        return {};
    }
    file >> width >> height;
    int maxVal;
    file >> maxVal;
    std::vector<Color> pixels(width * height);
    for (int i = 0; i < width * height; ++i)
    {
        int r, g, b;
        file >> r >> g >> b;
        pixels[i] = Color(r / 255.0, g / 255.0, b / 255.0);
    }
    return pixels;
}

static DenoiserOwner tryLoadOIDN()
{
    void *h = dlopen(std::string(ODIN_DENOISER_LIB_PATH).c_str(), RTLD_LAZY);
    if (!h)
        return {};
    auto create = (CreateDenoiserFunc)dlsym(h, "create_denoiser");
    auto destroy = (DestroyDenoiserFunc)dlsym(h, "destroy_denoiser");
    if (!create || !destroy)
    {
        dlclose(h);
        return {};
    }
    IDenoiser *d = create();
    if (!d || !d->isAvailable())
    {
        if (d)
            destroy(d);
        dlclose(h);
        return {};
    }
    std::cout << "Using OIDN denoiser" << std::endl;
    return DenoiserOwner(d, destroy, h);
}

static DenoiserOwner tryLoadSimple()
{
    void *h = dlopen(std::string(SIMPLE_DENOISER_LIB_PATH).c_str(), RTLD_LAZY);
    if (!h)
    {
        std::cerr << "Failed to load simple denoiser: " << dlerror() << std::endl;
        return {};
    }
    auto create = (CreateDenoiserFunc)dlsym(h, "create_simple_denoiser");
    auto destroy = (DestroyDenoiserFunc)dlsym(h, "destroy_simple_denoiser");
    if (!create || !destroy)
    {
        std::cerr << "Failed to load simple denoiser functions" << std::endl;
        dlclose(h);
        return {};
    }
    IDenoiser *d = create();
    if (!d)
    {
        std::cerr << "Failed to create simple denoiser" << std::endl;
        dlclose(h);
        return {};
    }
    return DenoiserOwner(d, destroy, h);
}

static DenoiserOwner loadDenoiser()
{
    DenoiserOwner d = tryLoadOIDN();
    if (d)
        return d;
    std::cout << "OIDN not available, falling back to simple denoiser" << std::endl;
    return tryLoadSimple();
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " <input.ppm> <output.ppm>" << std::endl;
        return 1;
    }
    DenoiserOwner dh = loadDenoiser();
    if (!dh)
    {
        std::cerr << "No denoiser available" << std::endl;
        return 1;
    }
    std::cout << "Using denoiser: " << dh.get()->getName() << std::endl;
    int width, height;
    std::cout << "Loading image: " << argv[1] << std::endl;
    std::vector<Color> noisyImage = loadPPM(argv[1], width, height);
    if (noisyImage.empty())
    {
        std::cerr << "Failed to load image" << std::endl;
        return 1;
    }
    std::cout << "Image size: " << width << "x" << height << std::endl;
    std::cout << "Denoising..." << std::endl;
    std::vector<Color> result = dh.get()->denoise(noisyImage, width, height);
    std::cout << "Saving denoised image: " << argv[2] << std::endl;
    PPMWriter::write(argv[2], result, width, height);
    std::cout << "Done!" << std::endl;
    return 0;
}
