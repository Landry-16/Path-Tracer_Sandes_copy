/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** Factory
*/

#ifndef RT_FACTORY_HPP
    #define RT_FACTORY_HPP

    #include <string>

    #include "rt/core/Registry.hpp"
    #include "rt/interfaces/IPrimitive.hpp"
    #include "rt/interfaces/ILight.hpp"
    #include "rt/interfaces/IMaterial.hpp"
    #include "rt/interfaces/ITexture.hpp"
    #include "rt/math/Vec3.hpp"
    #include "rt/math/Color.hpp"
    #include "rt/math/Matrix4.hpp"

struct TextureParams {
    Color color1 = Color(1.0, 1.0, 1.0);
    Color color2 = Color(0.0, 0.0, 0.0);
    double scale = 1.0;
    std::string imagePath;
};

struct MaterialParams {
    Color color = Color(1.0, 1.0, 1.0);
    double reflectivity = 0.0;
    double refractiveIndex = 1.0;
    double emissionStrength = 0.0;
    Color specular = Color(0.0, 0.0, 0.0);
    double shininess = 0.0;
    double roughness = 0.5;
    bool hasSpecular = false;
    bool hasTexture = false;
    std::string textureType;
    TextureParams textureParams;
};

struct PrimitiveParams {
    Vec3 position;
    Vec3 direction;
    double radius;
    std::shared_ptr<const IMaterial> material;
    Matrix4 transform;
    std::string objPath;
    bool useFileHeader = false;
    std::string materialType;
    MaterialParams materialParams;
};

struct LightParams {
    Vec3 position;
    Vec3 direction;
    Color color;
    double intensity;
};

using PrimitiveRegistry = Registry<IPrimitive, const PrimitiveParams&>;
using LightRegistry = Registry<ILight, const LightParams&>;
using MaterialRegistry = Registry<IMaterial, const MaterialParams&>;
using TextureRegistry = Registry<ITexture, const TextureParams&>;

class Factory {
public:
    static std::unique_ptr<IPrimitive> createPrimitive(const std::string &type, const PrimitiveParams &params) {
        return PrimitiveRegistry::instance().create(type, params);
    }
    
    static std::unique_ptr<ILight> createLight(const std::string &type, const LightParams &params) {
        return LightRegistry::instance().create(type, params);
    }
    
    static std::unique_ptr<IMaterial> createMaterial(const std::string &type, const MaterialParams &params) {
        return MaterialRegistry::instance().create(type, params);
    }
    
    static std::unique_ptr<ITexture> createTexture(const std::string &type, const TextureParams &params) {
        return TextureRegistry::instance().create(type, params);
    }
    
    static bool isPrimitiveRegistered(const std::string &type) {
        return PrimitiveRegistry::instance().isRegistered(type);
    }
    
    static bool isLightRegistered(const std::string &type) {
        return LightRegistry::instance().isRegistered(type);
    }
    
    static bool isMaterialRegistered(const std::string &type) {
        return MaterialRegistry::instance().isRegistered(type);
    }
    
    static bool isTextureRegistered(const std::string &type) {
        return TextureRegistry::instance().isRegistered(type);
    }
    
    static void clearAllRegistries() {
        PrimitiveRegistry::instance().clear();
        LightRegistry::instance().clear();
        MaterialRegistry::instance().clear();
        TextureRegistry::instance().clear();
    }
};

#endif // RT_FACTORY_HPP
