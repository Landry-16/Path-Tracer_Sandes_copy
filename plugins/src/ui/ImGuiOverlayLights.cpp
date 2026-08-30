/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** ImGuiOverlay lights panel
*/

#include <imgui.h>
#include <algorithm>

#include "ImGuiOverlay.hpp"
#include "ImGuiOverlayConfig.hpp"
#include "rt/core/SceneContext.hpp"

void ImGuiOverlay::readLightState(
    int i, ILight *light, float col[3], float &intensity)
{
    auto *lcfg = getLight(i);
    if (lcfg) {
        cfgGetColor(*lcfg, "color", col, 1.f);
        intensity = cfgGetFloat(*lcfg, "intensity", 1.f);
        return;
    }
    Color lc = light->getColor();
    col[0] = (float)std::clamp(lc.x, 0.0, 1.0);
    col[1] = (float)std::clamp(lc.y, 0.0, 1.0);
    col[2] = (float)std::clamp(lc.z, 0.0, 1.0);
}

void ImGuiOverlay::applyLightColor(
    int i, ILight *light, const float col[3], float intensity)
{
    light->setColor(Color(col[0], col[1], col[2]));
    light->setBaseIntensity(intensity);
    SceneContext::instance().triggerRerender();
    auto *lcfg = getLight(i);
    if (!lcfg)
        return;
    cfgSetColorArr(*lcfg, "color", col);
    cfgSetFloat(*lcfg, "intensity", intensity);
    dirty_ = true;
}

void ImGuiOverlay::buildLightPosFields(int i, const std::string &kind)
{
    auto *lcfg = getLight(i);
    if (!lcfg)
        return;
    if (kind == "directional" && lcfg->exists("direction")) {
        float dir[3];
        cfgGetColor(*lcfg, "direction", dir, 0.f);
        if (ImGui::DragFloat3("Direction##ld", dir, 0.01f, -1.f, 1.f)) {
            cfgSetColorArr(*lcfg, "direction", dir);
            dirty_ = true;
        }
    }
    if (kind == "point" && lcfg->exists("position")) {
        float pos[3];
        cfgGetColor(*lcfg, "position", pos, 0.f);
        if (ImGui::DragFloat3("Position##lp", pos, 0.1f)) {
            cfgSetColorArr(*lcfg, "position", pos);
            dirty_ = true;
        }
    }
}

bool ImGuiOverlay::buildLightEntry(int i)
{
    ILight *light = SceneContext::instance().getMutableScene()->lights[i].get();
    std::string kind = getLightType(i);
    char header[64];
    snprintf(header, sizeof(header), "#%d  [%s]", i, kind.c_str());
    ImGui::PushID(i);
    if (!ImGui::CollapsingHeader(header)) {
        ImGui::PopID();
        return false;
    }
    float col[3] = {1, 1, 1};
    float intensity = 1.f;
    readLightState(i, light, col, intensity);
    bool lc = ImGui::ColorEdit3("Color##lc", col);
    bool li = ImGui::SliderFloat("Intensity##li", &intensity, 0.f, 5.f);
    if (lc || li)
        applyLightColor(i, light, col, intensity);
    buildLightPosFields(i, kind);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.1f, 1.f));
    bool doRemove = ImGui::SmallButton("Remove##rl");
    ImGui::PopStyleColor();
    ImGui::PopID();
    if (doRemove)
        removeLight(i);
    return doRemove;
}

void ImGuiOverlay::buildLightsPanel()
{
    Scene *scene = SceneContext::instance().getMutableScene();
    ImGui::SetNextWindowPos(ImVec2(360.f, 40.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(310.f, 340.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Lights");
    ImGui::Text("Lights (%d)", scene ? (int)scene->lights.size() : 0);
    ImGui::SameLine();
    if (ImGui::SmallButton("+##addlight"))
        openAddLightModal_ = true;
    if (!scene || scene->lights.empty()) {
        ImGui::TextDisabled("(none)");
        ImGui::End();
        return;
    }
    for (int i = 0; i < (int)scene->lights.size(); ++i)
        if (buildLightEntry(i)) { ImGui::End(); return; }
    ImGui::End();
}
