/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** ImGuiOverlay scene data access, picking, sync buffers, primitive/light CRUD
*/

#include <algorithm>

#include "ImGuiOverlay.hpp"
#include "ImGuiOverlayConfig.hpp"
#include "rt/core/SceneContext.hpp"
#include "rt/math/Ray.hpp"

libconfig::Setting *ImGuiOverlay::getObject(int idx)
{
    if (!configLoaded_)
        return nullptr;
    try {
        auto &objs = cfg_.lookup("objects");
        if (idx < 0 || idx >= objs.getLength())
            return nullptr;
        return &objs[idx];
    } catch (...) { return nullptr; }
}

libconfig::Setting *ImGuiOverlay::getLight(int idx)
{
    if (!configLoaded_)
        return nullptr;
    try {
        auto &lights = cfg_.lookup("lights");
        if (idx < 0 || idx >= lights.getLength())
            return nullptr;
        return &lights[idx];
    } catch (...) { return nullptr; }
}

std::string ImGuiOverlay::getObjType(int idx)
{
    auto *obj = getObject(idx);
    if (!obj || !obj->exists("type"))
        return "unknown";
    return (const char *)(*obj)["type"];
}

std::string ImGuiOverlay::getLightType(int idx)
{
    auto *l = getLight(idx);
    if (!l || !l->exists("type"))
        return "unknown";
    return (const char *)(*l)["type"];
}

std::string ImGuiOverlay::getMatType(int idx)
{
    auto *obj = getObject(idx);
    if (!obj)
        return "flat";
    return obj->exists("material") ? (const char *)(*obj)["material"] : "flat";
}

void ImGuiOverlay::syncEditFromMaterial(const IMaterial &mat)
{
    Color dc = mat.getDiffuse();
    editColor_[0] = (float)std::clamp(dc.x, 0.0, 1.0);
    editColor_[1] = (float)std::clamp(dc.y, 0.0, 1.0);
    editColor_[2] = (float)std::clamp(dc.z, 0.0, 1.0);
    Color em = mat.getEmission();
    editEmission_[0] = (float)std::clamp(em.x, 0.0, 1.0);
    editEmission_[1] = (float)std::clamp(em.y, 0.0, 1.0);
    editEmission_[2] = (float)std::clamp(em.z, 0.0, 1.0);
    editReflectivity_ = (float)mat.getReflectivity();
    editRoughness_ = (float)mat.getRoughness();
    editMetallic_ = (float)mat.getMetallic();
    double ior = mat.getRefractiveIndex();
    editIOR_ = (float)(ior > 0 ? ior : 1.5);
}

void ImGuiOverlay::syncEditFromConfig(libconfig::Setting &obj)
{
    cfgGetColor(obj, "color", editColor_, 1.f);
    cfgGetColor(obj, "emission", editEmission_, 0.f);
    editReflectivity_ = cfgGetFloat(obj, "reflectivity", 0.f);
    editRoughness_ = cfgGetFloat(obj, "roughness", 0.5f);
    editMetallic_ = cfgGetFloat(obj, "metallic", 0.f);
    editIOR_ = cfgGetFloat(obj, "refractiveIndex", 1.5f);
}

void ImGuiOverlay::syncEditBuffers()
{
    if (pick_.material) {
        syncEditFromMaterial(*pick_.material);
        return;
    }
    if (!configLoaded_ || pick_.primitiveIndex < 0)
        return;
    auto *obj = getObject(pick_.primitiveIndex);
    if (obj)
        syncEditFromConfig(*obj);
}

void ImGuiOverlay::syncPlaneBuffers(libconfig::Setting &obj)
{
    if (obj.exists("point")) {
        editPoint_[0] = (float)(double)obj["point"][0];
        editPoint_[1] = (float)(double)obj["point"][1];
        editPoint_[2] = (float)(double)obj["point"][2];
    }
    if (obj.exists("normal")) {
        editNormal_[0] = (float)(double)obj["normal"][0];
        editNormal_[1] = (float)(double)obj["normal"][1];
        editNormal_[2] = (float)(double)obj["normal"][2];
    }
}

void ImGuiOverlay::syncGeometryBuffers()
{
    if (!configLoaded_ || pick_.primitiveIndex < 0)
        return;
    auto *obj = getObject(pick_.primitiveIndex);
    if (!obj)
        return;
    std::string type = getObjType(pick_.primitiveIndex);
    if (type == "sphere") {
        cfgGetColor(*obj, "center", editCenter_, 0.f);
        editRadius_ = cfgGetFloat(*obj, "radius", 1.f);
    } else if (type == "plane") {
        syncPlaneBuffers(*obj);
    }
}

void ImGuiOverlay::selectPrimitive(int idx)
{
    pick_.primitiveIndex = idx;
    pick_.material = nullptr;
    syncEditBuffers();
    syncGeometryBuffers();
}

void ImGuiOverlay::applyPickResult(int idx, const HitRecord &hit)
{
    pick_.primitiveIndex = idx;
    if (idx < 0)
        return;
    pick_.hitPoint = hit.point;
    pick_.hitNormal = hit.normal;
    pick_.hitT = hit.t;
    pick_.material = hit.material;
    syncEditBuffers();
    syncGeometryBuffers();
}

void ImGuiOverlay::tryPick(int mouseX, int mouseY)
{
    const Scene *scene = SceneContext::instance().getScene();
    if (!scene || !scene->camera)
        return;
    Ray ray = scene->camera->generateRay(mouseX, scene->height - 1 - mouseY);
    double tMax = 1e9;
    int bestIdx = -1;
    HitRecord bestHit{};
    for (int i = 0; i < (int)scene->primitives.size(); ++i) {
        HitRecord hit{};
        if (scene->primitives[i]->intersect(ray, 1e-4, tMax, hit))
            { tMax = hit.t; bestIdx = i; bestHit = hit; }
    }
    applyPickResult(bestIdx, bestHit);
}

void ImGuiOverlay::removePrimitive(int idx)
{
    if (!configLoaded_)
        return;
    try {
        cfg_.lookup("objects").remove(idx);
        pick_ = PickResult{};
        saveAndReload();
    } catch (...) {}
}

void ImGuiOverlay::addPrimGeometry(libconfig::Setting &obj)
{
    cfgAddVec3(obj, "center", newPrimPos_);
    obj.add("radius", libconfig::Setting::TypeFloat) = (double)newPrimRadius_;
    if (newPrimType_ == 7) {
        cfgAddVec3(obj, "direction", newPrimTorusDir_);
        cfgAddVec3(obj, "rotation", newPrimTorusRot_);
    }
}

void ImGuiOverlay::addPrimMaterial(libconfig::Setting &obj)
{
    if (newPrimMatType_ != 1)
        return;
    obj.add("material", libconfig::Setting::TypeString) = "reflective";
    obj.add("reflectivity", libconfig::Setting::TypeFloat) = (double)newPrimReflectivity_;
}

void ImGuiOverlay::addPrimitive()
{
    if (!configLoaded_)
        return;
    static const char *typeNames[] = {
        "sphere", "plane", "box", "cylinder",
        "cone", "pyramid", "triangle", "torus"
    };
    try {
        auto &obj = cfg_.lookup("objects").add(libconfig::Setting::TypeGroup);
        obj.add("type", libconfig::Setting::TypeString) = typeNames[newPrimType_];
        if (newPrimType_ == 1) {
            cfgAddVec3(obj, "point", newPrimPos_);
            cfgAddVec3(obj, "normal", newPrimNormal_);
        } else {
            addPrimGeometry(obj);
        }
        cfgAddVec3(obj, "color", newPrimColor_);
        addPrimMaterial(obj);
        saveAndReload();
    } catch (...) {}
}

void ImGuiOverlay::removeLight(int idx)
{
    if (!configLoaded_)
        return;
    try {
        cfg_.lookup("lights").remove(idx);
        saveAndReload();
    } catch (...) {}
}

void ImGuiOverlay::addLight()
{
    if (!configLoaded_)
        return;
    static const char *typeNames[] = {"ambient", "directional", "point"};
    try {
        auto &light = cfg_.lookup("lights").add(libconfig::Setting::TypeGroup);
        light.add("type", libconfig::Setting::TypeString) = typeNames[newLightType_];
        cfgAddVec3(light, "color", newLightColor_);
        light.add("intensity", libconfig::Setting::TypeFloat) = (double)newLightIntensity_;
        if (newLightType_ == 1) cfgAddVec3(light, "direction", newLightDir_);
        if (newLightType_ == 2) cfgAddVec3(light, "position", newLightPos_);
        saveAndReload();
    } catch (...) {}
}
