/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** PluginManager
*/

#ifndef RT_PLUGINMANAGER_HPP
    #define RT_PLUGINMANAGER_HPP

    #include <string>
    #include <vector>
    #include <memory>
    #include <functional>

class PluginManager {
public:
    using PluginHandle = void*;
    using RegisterFunc = void(*)();
    
    static PluginManager &instance() {
        static PluginManager manager;
        return manager;
    }
    
    bool loadPlugin(const std::string &path);
    void loadPluginsFromDirectory(const std::string &directory);
    void unloadAll();
    
    const std::vector<std::string> &getLoadedPlugins() const {
        return loadedPlugins;
    }
    
    ~PluginManager();

private:
    PluginManager() = default;
    PluginManager(const PluginManager&) = delete;
    PluginManager &operator=(const PluginManager&) = delete;
    
    std::vector<PluginHandle> handles;
    std::vector<std::string> loadedPlugins;
};

#endif // RT_PLUGINMANAGER_HPP
