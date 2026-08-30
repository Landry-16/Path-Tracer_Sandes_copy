/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** ImGuiOverlay scene inspector, geometry editor, material editor
*/

#include <imgui.h>
#include <cstring>

#include "ImGuiOverlay.hpp"
#include "ImGuiOverlayConfig.hpp"
#include "rt/core/SceneContext.hpp"

void ImGuiOverlay::buildInspectorHeader(const Scene *scene)
{
    if (scene)
        ImGui::Text("Prims: %d  Lights: %d  %dx%d",
            (int)scene->primitives.size(), (int)scene->lights.size(),
            scene->width, scene->height);
    else
        ImGui::TextDisabled("No scene loaded");
    if (dirty_)
        ImGui::SameLine(), ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), " *");
    ImGui::SameLine(0, 10);
    if (ImGui::SmallButton("Save & Reload"))
        saveAndReload();
    ImGui::Separator();
}

void ImGuiOverlay::buildObjectList()
{
    auto &objs = cfg_.lookup("objects");
    int n = objs.getLength();
    ImGui::Text("Objects (%d)", n);
    ImGui::SameLine();
    if (ImGui::SmallButton("+##addprim"))
        openAddPrimModal_ = true;
    ImGui::BeginChild("obj_list", ImVec2(0, 85), ImGuiChildFlags_Border);
    for (int i = 0; i < n; ++i) {
        auto &obj   = objs[i];
        const char *tp = obj.exists("type") ? (const char *)obj["type"] : "?";
        char lbl[64];
        snprintf(lbl, sizeof(lbl), "#%d  [%s]", i, tp);
        if (ImGui::Selectable(lbl, pick_.primitiveIndex == i))
            selectPrimitive(i);
    }
    ImGui::EndChild();
}

void ImGuiOverlay::buildInspectorPanel()
{
    const Scene *scene = SceneContext::instance().getScene();
    ImGui::SetNextWindowPos(ImVec2(10.f, 40.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(340.f, 540.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Scene Inspector");
    buildInspectorHeader(scene);
    if (configLoaded_) {
        try { buildObjectList(); } catch (...) {}
    }
    ImGui::Separator();
    if (pick_.primitiveIndex < 0)
        ImGui::TextDisabled("Click scene or select above.");
    else
        buildObjectEditor(scene);
    ImGui::End();
}

bool ImGuiOverlay::buildPrimitiveHeader(int idx)
{
    ImGui::Text("Primitive #%d", idx);
    if (pick_.hitT > 0.0)
        ImGui::TextDisabled("  pos(%.2f,%.2f,%.2f)  d=%.2f",
            pick_.hitPoint.x, pick_.hitPoint.y,
            pick_.hitPoint.z, pick_.hitT);
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.1f, 1.f));
    bool doRemove = ImGui::SmallButton("Remove##pr");
    ImGui::PopStyleColor();
    if (doRemove)
        removePrimitive(idx);
    return doRemove;
}

void ImGuiOverlay::buildObjectEditor(const Scene *scene)
{
    (void)scene;
    int idx = pick_.primitiveIndex;
    if (buildPrimitiveHeader(idx))
        return;
    buildGeometryEditor(idx);
    buildMaterialEditor(idx);
}

void ImGuiOverlay::applySphereGeo(libconfig::Setting &obj)
{
    cfgSetColorArr(obj, "center", editCenter_);
    cfgSetFloat(obj, "radius", editRadius_);
    dirty_ = true;
}

void ImGuiOverlay::applyPlaneGeo(libconfig::Setting &obj)
{
    cfgSetColorArr(obj, "point", editPoint_);
    cfgSetColorArr(obj, "normal", editNormal_);
    dirty_ = true;
}

void ImGuiOverlay::buildGeometryEditor(int idx)
{
    ImGui::Spacing();
    ImGui::SeparatorText("Geometry");
    auto *obj = getObject(idx);
    if (!obj)
        return;
    std::string type = getObjType(idx);
    ImGui::TextDisabled("Type: %s", type.c_str());
    if (type == "sphere") {
        bool c = ImGui::DragFloat3(
            "Center##geo", editCenter_, 0.05f, -1e6f, 1e6f, "%.3f");
        c |= ImGui::DragFloat(
            "Radius##geo", &editRadius_, 0.02f, 0.001f, 1000.f, "%.3f");
        if (c) applySphereGeo(*obj);
    } else if (type == "plane") {
        bool c = ImGui::DragFloat3(
            "Point##geo", editPoint_, 0.05f, -1e6f, 1e6f, "%.3f");
        c |= ImGui::DragFloat3(
            "Normal##geo", editNormal_, 0.01f, -1.f, 1.f, "%.3f");
        if (c) applyPlaneGeo(*obj);
    } else {
        ImGui::TextDisabled("(geometry editing not supported for this type)");
    }
}

void ImGuiOverlay::initMatDefaults(libconfig::Setting &obj, const char *matName)
{
    if (strcmp(matName, "reflective") == 0 && !obj.exists("reflectivity"))
        obj.add("reflectivity", libconfig::Setting::TypeFloat) = 0.8;
    if (strcmp(matName, "pbr") == 0) {
        if (!obj.exists("roughness"))
            obj.add("roughness", libconfig::Setting::TypeFloat) = 0.5;
        if (!obj.exists("metallic"))
            obj.add("metallic",  libconfig::Setting::TypeFloat) = 0.0;
    }
    if (strcmp(matName, "transparent") == 0) {
        if (!obj.exists("refractiveIndex"))
            obj.add("refractiveIndex", libconfig::Setting::TypeFloat) = 1.5;
        if (!obj.exists("reflectivity"))
            obj.add("reflectivity", libconfig::Setting::TypeFloat) = 0.9;
    }
}

bool ImGuiOverlay::buildMatTypeCombo(
    libconfig::Setting *obj, const std::string &mt,
    const char **matTypes, int nMat)
{
    int selMat = 0;
    for (int i = 0; i < nMat; ++i)
        if (mt == matTypes[i]) { selMat = i; break; }
    ImGui::SetNextItemWidth(150.f);
    if (!ImGui::Combo("Type##mt", &selMat, matTypes, nMat) || !obj)
        return false;
    cfgSetString(*obj, "material", matTypes[selMat]);
    initMatDefaults(*obj, matTypes[selMat]);
    dirty_ = true;
    saveAndReload();
    return true;
}

bool ImGuiOverlay::buildMatSliders(const std::string &mt)
{
    bool changed = false;
    if (mt == "reflective" || mt == "glossy" || mt == "pbr" || mt == "transparent")
        changed |= ImGui::SliderFloat("Reflectivity", &editReflectivity_, 0.f, 1.f);
    if (mt == "pbr" || mt == "glossy") {
        changed |= ImGui::SliderFloat("Roughness", &editRoughness_, 0.f, 1.f);
        if (mt == "pbr")
            changed |= ImGui::SliderFloat("Metallic", &editMetallic_, 0.f, 1.f);
    }
    if (mt == "transparent")
        changed |= ImGui::SliderFloat("IOR", &editIOR_, 1.f, 3.f);
    return changed;
}

void ImGuiOverlay::pushMatToLive(IMaterial *mmat)
{
    if (!mmat)
        return;
    mmat->setColor(Color(editColor_[0], editColor_[1], editColor_[2]));
    mmat->setEmission(Color(editEmission_[0], editEmission_[1], editEmission_[2]));
    mmat->setReflectivity(editReflectivity_);
    mmat->setRoughness(editRoughness_);
    mmat->setMetallic(editMetallic_);
    mmat->setRefractiveIndex(editIOR_);
    SceneContext::instance().triggerRerender();
}

void ImGuiOverlay::pushMatToConfig(libconfig::Setting &obj, const std::string &mt)
{
    cfgSetColorArr(obj, "color", editColor_);
    if (editEmission_[0] > 0.f || editEmission_[1] > 0.f || editEmission_[2] > 0.f)
        cfgSetColorArr(obj, "emission", editEmission_);
    if (mt == "reflective" || mt == "glossy" || mt == "pbr" || mt == "transparent")
        cfgSetFloat(obj, "reflectivity", editReflectivity_);
    if (mt == "pbr" || mt == "glossy") {
        cfgSetFloat(obj, "roughness", editRoughness_);
        cfgSetFloat(obj, "metallic",  editMetallic_);
    }
    if (mt == "transparent")
        cfgSetFloat(obj, "refractiveIndex", editIOR_);
    dirty_ = true;
}

void ImGuiOverlay::buildMaterialEditor(int idx)
{
    ImGui::Spacing();
    ImGui::SeparatorText("Material");
    auto *obj = getObject(idx);
    std::string mt = getMatType(idx);
    static const char *matTypes[] = {
        "flat", "reflective", "pbr", "glossy", "emissive", "transparent"
    };
    if (buildMatTypeCombo(obj, mt, matTypes, 6))
        return;
    IMaterial *mmat = pick_.material
        ? const_cast<IMaterial *>(pick_.material.get()) : nullptr;
    bool changed = ImGui::ColorEdit3("Diffuse",  editColor_);
    changed |= ImGui::ColorEdit3("Emission", editEmission_);
    changed |= buildMatSliders(mt);
    if (!changed)
        return;
    pushMatToLive(mmat);
    if (obj)
        pushMatToConfig(*obj, mt);
}
