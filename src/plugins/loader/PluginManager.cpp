/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** PluginManager
*/

#include "rt/core/PluginManager.hpp"
#include <dlfcn.h>
#include <iostream>
#include <filesystem>

static void *openLibrary(const std::string &path)
{
    void *handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (!handle)
        std::cerr << "Failed to load plugin " << path << ": " << dlerror() << std::endl;
    return handle;
}

static PluginManager::RegisterFunc resolveRegisterFunc(void *handle, const std::string &path)
{
    dlerror();
    auto func = reinterpret_cast<PluginManager::RegisterFunc>(dlsym(handle, "rt_plugin_register"));
    const char *err = dlerror();
    if (err)
    {
        std::cerr << "Failed to find rt_plugin_register in " << path << ": " << err << std::endl;
        dlclose(handle);
        return nullptr;
    }
    return func;
}

static bool callRegisterFunc(void *handle, const std::string &path,
    PluginManager::RegisterFunc registerFunc)
{
    try
    {
        registerFunc();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Plugin registration failed for " << path << ": " << e.what() << std::endl;
        dlclose(handle);
        return false;
    }
    return true;
}

bool PluginManager::loadPlugin(const std::string &path)
{
    void *handle = openLibrary(path);
    if (!handle)
        return false;
    RegisterFunc registerFunc = resolveRegisterFunc(handle, path);
    if (!registerFunc)
        return false;
    if (!callRegisterFunc(handle, path, registerFunc))
        return false;
    handles.push_back(handle);
    loadedPlugins.push_back(path);
    std::cout << "Loaded plugin: " << path << std::endl;
    return true;
}

void PluginManager::loadPluginsFromDirectory(const std::string &directory)
{
    namespace fs = std::filesystem;
    if (!fs::exists(directory) || !fs::is_directory(directory))
    {
        std::cerr << "Plugin directory does not exist: " << directory << std::endl;
        return;
    }
    for (const auto &entry : fs::directory_iterator(directory))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".so")
            loadPlugin(entry.path().string());
    }
}

void PluginManager::unloadAll()
{
    for (auto it = handles.rbegin(); it != handles.rend(); ++it)
    {
        if (*it)
            dlclose(*it);
    }
    handles.clear();
    loadedPlugins.clear();
}

PluginManager::~PluginManager()
{
    unloadAll();
}
