/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** MTL file loader and material finalization
*/

#include "OBJMesh.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <set>
#include "rt/core/Factory.hpp"
#include "rt/materials/FlatMaterial.hpp"
#include "rt/core/Registry.hpp"

using ColorSetter = std::function<void(MTLMaterial&, double, double, double)>;
using FloatSetter = std::function<void(MTLMaterial&, double)>;

static const std::map<std::string, ColorSetter> s_colorSetters = {
    {"Ka", [](MTLMaterial &m, double r, double g, double b) { m.ambient  = Color(r, g, b); }},
    {"Kd", [](MTLMaterial &m, double r, double g, double b) { m.diffuse  = Color(r, g, b); }},
    {"Ks", [](MTLMaterial &m, double r, double g, double b) { m.specular = Color(r, g, b); }},
};

static const std::map<std::string, FloatSetter> s_floatSetters = {
    {"Ns", [](MTLMaterial &m, double v) { m.shininess = v; }},
    {"d",  [](MTLMaterial &m, double v) { (void)m; (void)v; }},
};

void OBJMesh::parseMTLColorOrFloat(const std::string &type, std::istringstream &iss,
    MTLMaterial &mtl)
{
    auto itC = s_colorSetters.find(type);
    if (itC != s_colorSetters.end())
    {
        double r, g, b;
        iss >> r >> g >> b;
        itC->second(mtl, r, g, b);
        return;
    }
    auto itF = s_floatSetters.find(type);
    if (itF != s_floatSetters.end())
    {
        double val;
        iss >> val;
        itF->second(mtl, val);
    }
}

void OBJMesh::parseMTLTextureProp(const std::string &type, std::istringstream &iss,
    MTLMaterial &mtl,
    const std::function<std::string(std::istringstream&)> &parseTex)
{
    std::string path = parseTex(iss);
    if (path.empty())
        return;

    static const std::map<std::string, std::string MTLMaterial::*> s_texFields = {
        {"map_Kd",   &MTLMaterial::diffuseTexturePath},
        {"map_Bump", &MTLMaterial::normalMapPath},
        {"bump",     &MTLMaterial::normalMapPath},
    };

    auto it = s_texFields.find(type);
    if (it != s_texFields.end())
        mtl.*(it->second) = resolveTexturePath(path);
}

void OBJMesh::parseMTLToken(const std::string &type, std::istringstream &iss,
    MTLMaterial*& current,
    const std::function<std::string(std::istringstream&)> &parseTex)
{
    if (type == "newmtl")
    {
        std::string name;
        iss >> name;
        materials[name] = MTLMaterial();
        materials[name].name = name;
        current = &materials[name];
        return;
    }
    if (!current)
        return;

    static const std::set<std::string> colorFloatTypes = {
        "Ka", "Kd", "Ks", "Ns", "d"
    };
    static const std::set<std::string> texTypes = {
        "map_Kd", "map_Bump", "bump"
    };

    if (colorFloatTypes.count(type))
        parseMTLColorOrFloat(type, iss, *current);
    else if (texTypes.count(type))
        parseMTLTextureProp(type, iss, *current, parseTex);
}

void OBJMesh::tryLoadTexturedMaterial(MTLMaterial &mtl)
{
    if (mtl.diffuseTexturePath.empty())
        return;
    if (!TextureRegistry::instance().isRegistered("image"))
        return;
    try
    {
        TextureParams params;
        params.imagePath = mtl.diffuseTexturePath;
        auto texture = TextureRegistry::instance().create("image", params);
        if (!texture)
            return;
        auto flat = std::make_shared<FlatMaterial>(Color(1, 1, 1), std::move(texture));
        flat->setSpecular(mtl.specular, mtl.shininess);
        ownedMaterials.push_back(flat);
        mtl.material = flat;
        std::cout << "[MTL] Loaded texture: " << mtl.diffuseTexturePath << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[MTL] Failed to load texture " << mtl.diffuseTexturePath
                  << ": " << e.what() << std::endl;
    }
}

void OBJMesh::finalizeMTLMaterial(MTLMaterial &mtl, const std::string &name)
{
    if (mtl.diffuseTexturePath.empty())
    {
        auto inferred = findFallbackDiffuseTexture(name);
        if (!inferred.empty())
        {
            std::cout << "[MTL] Inferred diffuse texture for " << name
                      << ": " << inferred << std::endl;
            mtl.diffuseTexturePath = inferred;
        }
    }
    tryLoadTexturedMaterial(mtl);
    if (!mtl.material)
    {
        auto flat = std::make_shared<FlatMaterial>(mtl.diffuse);
        flat->setSpecular(mtl.specular, mtl.shininess);
        ownedMaterials.push_back(flat);
        mtl.material = flat;
    }
}

bool OBJMesh::loadMTL(const std::string &mtlPath)
{
    std::ifstream file(mtlPath);
    if (!file.is_open())
    {
        std::cerr << "[MTL] Failed to open: " << mtlPath << std::endl;
        return false;
    }
    MTLMaterial *currentMaterial = nullptr;
    auto parseTexturePath = [&](std::istringstream &stream) -> std::string
    {
        std::string rem;
        return std::getline(stream >> std::ws, rem) ? trim(rem) : "";
    };
    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        parseMTLToken(type, iss, currentMaterial, parseTexturePath);
    }
    file.close();
    for (auto &[name, mtl] : materials)
        finalizeMTLMaterial(mtl, name);
    std::cout << "[MTL] Loaded " << materials.size() << " materials from "
              << mtlPath << std::endl;
    return true;
}
