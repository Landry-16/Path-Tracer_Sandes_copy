/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** LibconfigLoader
*/

#include "rt/scene/LibconfigLoader.hpp"
#include "rt/core/Factory.hpp"
#include "rt/math/Matrix4.hpp"
#include <stdexcept>
#include <iostream>
#include <cmath>

template<typename T>
static T readOr(const libconfig::Setting &s, const char *key, T def)
{
    T result = def;

    if (s.exists(key))
        result = static_cast<T>(s[key]);
    return result;
}

static std::string readStringOr(const libconfig::Setting &s, const char *key, const std::string &def)
{
    std::string result = def;

    if (s.exists(key))
        result = std::string(s[key].c_str());
    return result;
}

std::unique_ptr<Scene> LibconfigLoader::loadScene(const std::string &filename)
{
    libconfig::Config cfg;

    try {
        cfg.readFile(filename.c_str());
    } catch (const libconfig::FileIOException&) {
        throw std::runtime_error("Error reading scene file");
    } catch (const libconfig::ParseException &e) {
        throw std::runtime_error(std::string("Parse error: ") + e.getError());
    }
    auto scene = std::make_unique<Scene>();
    try {
        const libconfig::Setting &root = cfg.getRoot();
        
        if (!root.exists("camera"))
            throw std::runtime_error("Missing camera section");
        loadCamera(root["camera"], *scene);
        if (root.exists("objects"))
            loadObjects(root["objects"], *scene);
        if (root.exists("lights"))
            loadLights(root["lights"], *scene);
            
    } catch (const libconfig::SettingNotFoundException &e) {
        throw std::runtime_error(std::string("Missing setting: ") + e.getPath());
    } catch (const libconfig::SettingTypeException &e) {
        throw std::runtime_error(std::string("Type error: ") + e.getPath());
    }
    return scene;
}

void LibconfigLoader::loadCamera(const libconfig::Setting &cameraSetting, Scene &scene)
{
    Vec3 position = readVec3(cameraSetting["position"]);
    Vec3 lookAt = readVec3(cameraSetting["lookAt"]);
    Vec3 up = readVec3(cameraSetting["up"]);
    double fov = cameraSetting["fov"];
    int width = readOr<int>(cameraSetting, "width", 800);
    int height = readOr<int>(cameraSetting, "height", 600);
    int aaSamples = readOr<int>(cameraSetting, "antialiasing", 1);
    double aperture = readOr<double>(cameraSetting, "aperture", 0.0);
    double focusDistance = readOr<double>(cameraSetting, "focus_distance", 1.0);

    scene.width = width;
    scene.height = height;
    scene.antialiasingSamples = aaSamples;
    scene.aperture = aperture;
    scene.focusDistance = focusDistance;
    scene.camera = std::make_unique<Camera>(position, lookAt, up, fov, width, height);
    if (aperture > 0.0)
        scene.camera->setDepthOfField(aperture, focusDistance);
}

bool LibconfigLoader::parseTextureSettings(const libconfig::Setting &obj, const Color &color,
                                           TextureParams &texOut, std::string &typeOut)
{
    bool result = false;

    if (obj.exists("texture")) {
        const libconfig::Setting& tex = obj["texture"];
        std::string texType = std::string(tex["type"].c_str());
        texOut.color1 = tex.exists("color1") ? readColor(tex["color1"]) : color;
        texOut.color2 = tex.exists("color2") ? readColor(tex["color2"]) : Color(0, 0, 0);
        texOut.scale = readOr<double>(tex, "scale", 1.0);
        texOut.imagePath = readStringOr(tex, "path", "");
        if (Factory::isTextureRegistered(texType)) {
            typeOut = texType;
            result = true;
        } else {
            std::cerr << "Warning: texture type '" << texType << "' not registered, skipping texture" << std::endl;
        }
    }
    return result;
}

MaterialParams LibconfigLoader::buildMaterialParams(const libconfig::Setting &obj, const Color &color,
                                                     const std::string &texType, bool hasTex,
                                                     const TextureParams &tp)
{
    MaterialParams p;

    p.color = color;
    p.reflectivity = readOr<double>(obj, "reflectivity", 0.0);
    p.refractiveIndex = readOr<double>(obj, "refractiveIndex", 1.0);
    p.emissionStrength = readOr<double>(obj, "emissionStrength", 10.0);
    p.specular = obj.exists("specular") ? readColor(obj["specular"]) : Color(0, 0, 0);
    p.shininess = readOr<double>(obj, "shininess", 0.0);
    p.hasSpecular = obj.exists("specular") || obj.exists("shininess");
    p.hasTexture = hasTex;
    p.textureType = texType;
    p.textureParams = tp;
    p.roughness = readOr<double>(obj, "roughness", p.roughness);
    return p;
}

std::shared_ptr<const IMaterial> LibconfigLoader::createObjectMaterial(
    const libconfig::Setting &obj, Scene &scene, const MaterialContext &ctx,
    std::string &outMatType, MaterialParams &outParams)
{
    bool hasExplicit = obj.exists("material");
    outMatType = hasExplicit ? std::string(obj["material"].c_str()) : "flat";
    outParams = buildMaterialParams(obj, ctx.color, ctx.texType, ctx.hasTex, ctx.tp);
    bool wantsOverride = !ctx.isOBJ || hasExplicit || ctx.hasTex || (ctx.isOBJ && ctx.hasColor);
    std::shared_ptr<const IMaterial> result;

    if (wantsOverride) {
        auto mat = std::shared_ptr<IMaterial>(Factory::createMaterial(outMatType, outParams));
        if (mat) {
            scene.materials.push_back(mat);
            result = mat;
        } else {
            std::cerr << "Warning: Unknown material type '" << outMatType << "', skipping" << std::endl;
        }
    }
    return result;
}

Matrix4 LibconfigLoader::parseObjectTransform(const libconfig::Setting &obj)
{
    Matrix4 transform = Matrix4::identity();

    if (obj.exists("scale")) {
        Vec3 s = readVec3(obj["scale"]);
        transform = Matrix4::scale(s.x, s.y, s.z) * transform;
    }
    if (obj.exists("rotation")) {
        Vec3 r = readVec3(obj["rotation"]);
        double radX = r.x * M_PI / 180.0;
        double radY = r.y * M_PI / 180.0;
        double radZ = r.z * M_PI / 180.0;
        transform = Matrix4::rotationZ(radZ) * Matrix4::rotationY(radY) *
                    Matrix4::rotationX(radX) * transform;
    }
    if (obj.exists("translation")) {
        Vec3 t = readVec3(obj["translation"]);
        transform = Matrix4::translation(t.x, t.y, t.z) * transform;
    }
    return transform;
}

void LibconfigLoader::createAndAddPrimitive(const libconfig::Setting &obj, const std::string &type,
                                            Scene &scene,
                                            const std::shared_ptr<const IMaterial> &matPtr,
                                            const Matrix4 &transform,
                                            const std::string &matType,
                                            const MaterialParams &matParams)
{
    Vec3 pos = readVec3Or(obj, "center", readVec3Or(obj, "point", Vec3(0, 0, 0)));
    Vec3 dir = readVec3Or(obj, "normal", Vec3(0, 1, 0));
    double radius = readOr<double>(obj, "radius", 1.0);
    std::string objPath = readStringOr(obj, "path", "");
    bool useFileHeader = readOr<bool>(obj, "use_stl_header", false);
    PrimitiveParams primParams{pos, dir, radius, matPtr, transform, objPath,
        useFileHeader, matType, matParams};

    if (Factory::isPrimitiveRegistered(type))
        scene.primitives.push_back(Factory::createPrimitive(type, primParams));
    else
        std::cerr << "Warning: Unknown primitive type '" << type << "', skipping" << std::endl;
}

void LibconfigLoader::loadObjects(const libconfig::Setting &objectsSetting, Scene &scene)
{
    for (int i = 0; i < objectsSetting.getLength(); ++i) {
        const libconfig::Setting &obj = objectsSetting[i];
        std::string type = obj["type"].c_str();
        bool isOBJ = (type == "obj");
        bool hasColor = obj.exists("color");
        Color color(1, 1, 1);
        if (hasColor)
            color = readColor(obj["color"]);
        TextureParams texParams;
        std::string textureType;
        bool hasTex = parseTextureSettings(obj, color, texParams, textureType);
        MaterialContext ctx{isOBJ, hasColor, color, textureType, hasTex, texParams};
        std::string matType;
        MaterialParams matParams;
        auto matPtr = createObjectMaterial(obj, scene, ctx, matType, matParams);
        Matrix4 transform = parseObjectTransform(obj);
        createAndAddPrimitive(obj, type, scene, matPtr, transform,
            matType, matParams);
    }
}

void LibconfigLoader::loadLights(const libconfig::Setting &lightsSetting, Scene &scene)
{
    for (int i = 0; i < lightsSetting.getLength(); ++i) {
        const libconfig::Setting &light = lightsSetting[i];
        std::string type = light["type"].c_str();
        Color color = readColor(light["color"]);
        double intensity = light["intensity"];
        Vec3 position = readVec3Or(light, "position", Vec3(0, 0, 0));
        Vec3 direction = readVec3Or(light, "direction", Vec3(0, -1, 0));
        LightParams lightParams{position, direction, color, intensity};

        if (Factory::isLightRegistered(type)) {
            auto lightObj = Factory::createLight(type, lightParams);
            scene.lights.push_back(std::move(lightObj));
        } else {
            std::cerr << "Warning: Unknown light type '" << type << "', skipping" << std::endl;
        }
    }
}

Vec3 LibconfigLoader::readVec3(const libconfig::Setting &setting)
{
    if (setting.getLength() != 3)
        throw std::runtime_error("Vec3 must have 3 components");
    return Vec3(setting[0], setting[1], setting[2]);
}

Color LibconfigLoader::readColor(const libconfig::Setting &setting)
{
    return readVec3(setting);
}

Vec3 LibconfigLoader::readVec3Or(const libconfig::Setting &s, const char *key, const Vec3 &def)
{
    Vec3 result = def;

    if (s.exists(key))
        result = readVec3(s[key]);
    return result;
}
