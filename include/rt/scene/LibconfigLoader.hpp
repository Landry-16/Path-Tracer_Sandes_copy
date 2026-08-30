/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** LibconfigLoader
*/

#ifndef RT_LIBCONFIGLOADER_HPP
    #define RT_LIBCONFIGLOADER_HPP

    #include "rt/interfaces/ISceneLoader.hpp"
    #include "rt/core/Factory.hpp"
    #include <libconfig.h++>

struct MaterialContext {
    bool isOBJ = false;
    bool hasColor = false;
    Color color = Color(1.0, 1.0, 1.0);
    std::string texType;
    bool hasTex = false;
    TextureParams tp;
};

class LibconfigLoader : public ISceneLoader {
public:
    std::unique_ptr<Scene> loadScene(const std::string &filename) override;

private:
    void loadCamera(const libconfig::Setting &cameraSetting, Scene &scene);
    void loadObjects(const libconfig::Setting &objectsSetting, Scene &scene);
    void loadLights(const libconfig::Setting &lightsSetting, Scene &scene);

    Vec3 readVec3(const libconfig::Setting &setting);
    Color readColor(const libconfig::Setting &setting);
    Vec3 readVec3Or(const libconfig::Setting &s, const char *key, const Vec3 &def);

    bool parseTextureSettings(const libconfig::Setting &obj, const Color &color,
                              TextureParams &texOut, std::string &typeOut);
    MaterialParams buildMaterialParams(const libconfig::Setting &obj, const Color &color,
                                       const std::string &texType, bool hasTex,
                                       const TextureParams &tp);
    std::shared_ptr<const IMaterial> createObjectMaterial(const libconfig::Setting &obj,
                                                          Scene &scene,
                                                          const MaterialContext &ctx,
                                                          std::string &outMatType,
                                                          MaterialParams &outParams);
    Matrix4 parseObjectTransform(const libconfig::Setting &obj);
    void createAndAddPrimitive(const libconfig::Setting &obj, const std::string &type,
                               Scene &scene,
                               const std::shared_ptr<const IMaterial> &matPtr,
                               const Matrix4 &transform,
                               const std::string &matType,
                               const MaterialParams &matParams);
};

#endif // RT_LIBCONFIGLOADER_HPP
