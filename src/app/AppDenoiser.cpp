/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** AppDenoiser
*/

#include "AppDenoiser.hpp"
#include <iostream>
#include <dlfcn.h>

typedef IDenoiser *(*CreateDenoiserFunc)();
typedef void (*DestroyDenoiserFunc)(IDenoiser*);

DenoiserPtr makeNullDenoiser()
{
    return {nullptr, [](IDenoiser*) {}};
}

static DenoiserPtr tryLoadOIDNDenoiser()
{
    void *handle = dlopen("./plugins/raytracer_oidn_denoiser.so", RTLD_LAZY);
    if (!handle)
        return makeNullDenoiser();
    auto create = (CreateDenoiserFunc)dlsym(handle, "create_denoiser");
    auto destroy = (DestroyDenoiserFunc)dlsym(handle, "destroy_denoiser");
    if (!create || !destroy)
    {
        dlclose(handle);
        return makeNullDenoiser();
    }
    IDenoiser *d = create();
    if (!d || !d->isAvailable())
    {
        if (d)
            destroy(d);
        dlclose(handle);
        return makeNullDenoiser();
    }
    std::cout << "Denoiser loaded: " << d->getName() << std::endl;
    return {d, [destroy, handle](IDenoiser *p) { if (p) { destroy(p); dlclose(handle); } }};
}

static DenoiserPtr tryLoadSimpleDenoiser()
{
    std::cout << "OIDN not available, falling back to simple denoiser" << std::endl;
    void *handle = dlopen("./plugins/raytracer_simple_denoiser.so", RTLD_LAZY);
    if (!handle)
    {
        std::cerr << "Simple denoiser plugin not available" << std::endl;
        return makeNullDenoiser();
    }
    auto create = (CreateDenoiserFunc)dlsym(handle, "create_simple_denoiser");
    auto destroy = (DestroyDenoiserFunc)dlsym(handle, "destroy_simple_denoiser");
    if (!create || !destroy)
    {
        std::cerr << "Failed to load simple denoiser functions" << std::endl;
        dlclose(handle);
        return makeNullDenoiser();
    }
    IDenoiser *d = create();
    if (!d)
    {
        std::cerr << "Failed to create simple denoiser" << std::endl;
        dlclose(handle);
        return makeNullDenoiser();
    }
    std::cout << "Denoiser loaded: " << d->getName() << std::endl;
    return {d, [destroy, handle](IDenoiser *p) { if (p) { destroy(p); dlclose(handle); } }};
}

DenoiserPtr loadDenoiser()
{
    DenoiserPtr d = tryLoadOIDNDenoiser();
    if (d)
        return d;
    return tryLoadSimpleDenoiser();
}
