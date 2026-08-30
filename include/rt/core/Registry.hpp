/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** Registry
*/

#ifndef RT_REGISTRY_HPP
    #define RT_REGISTRY_HPP

    #include <string>
    #include <map>
    #include <memory>
    #include <functional>
    #include <stdexcept>

template<typename Base, typename... Args>
class Registry {
public:
    using CreatorFunc = std::function<std::unique_ptr<Base>(Args...)>;
    
    static Registry &instance() {
        static Registry registry;
        return registry;
    }
    
    void clear() {
        creators.clear();
    }
    
    void registerType(const std::string &name, CreatorFunc creator) {
        if (creators.find(name) != creators.end()) {
            throw std::runtime_error("Type already registered: " + name);
        }
        creators[name] = creator;
    }
    
    std::unique_ptr<Base> create(const std::string &name, Args... args) {
        auto it = creators.find(name);
        if (it == creators.end()) {
            throw std::runtime_error("Unknown type: " + name);
        }
        return it->second(std::forward<Args>(args)...);
    }
    
    bool isRegistered(const std::string &name) const {
        return creators.find(name) != creators.end();
    }
    
    std::vector<std::string> getRegisteredTypes() const {
        std::vector<std::string> types;
        for (const auto &pair : creators) {
            types.push_back(pair.first);
        }
        return types;
    }
    
private:
    Registry() = default;
    std::map<std::string, CreatorFunc> creators;
};

#endif // RT_REGISTRY_HPP
