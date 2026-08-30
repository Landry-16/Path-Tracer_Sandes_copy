/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** ImGuiOverlay camera panel
*/

#include <imgui.h>

#include "ImGuiOverlay.hpp"
#include "rt/core/SceneContext.hpp"

void ImGuiOverlay::buildCamMoveButtons(Camera &cam)
{
    auto mv = [&](const char *lbl, auto fn) {
        if (ImGui::Button(lbl)) {
            fn();
            SceneContext::instance().triggerRerender();
        }
        ImGui::SameLine();
    };
    mv("Fwd", [&]{ cam.moveForward(moveStep_); });
    mv("Back", [&]{ cam.moveBackward(moveStep_); });
    mv("Left", [&]{ cam.moveLeft(moveStep_); });
    mv("Right",[&]{ cam.moveRight(moveStep_); });
    mv("Up", [&]{ cam.moveUp(moveStep_); });
    if (ImGui::Button("Down")) {
        cam.moveDown(moveStep_);
        SceneContext::instance().triggerRerender();
    }
}

void ImGuiOverlay::buildCamRotateButtons(Camera &cam)
{
    auto rot = [&](const char *lbl, auto fn) {
        if (ImGui::Button(lbl)) {
            fn();
            SceneContext::instance().triggerRerender();
        }
        ImGui::SameLine();
    };
    rot("Yaw L", [&]{ cam.rotateYaw(-rotateDeg_); });
    rot("Yaw R", [&]{ cam.rotateYaw(rotateDeg_); });
    rot("Pch U", [&]{ cam.rotatePitch(rotateDeg_); });
    if (ImGui::Button("Pch D")) {
        cam.rotatePitch(-rotateDeg_);
        SceneContext::instance().triggerRerender();
    }
}

void ImGuiOverlay::buildCameraPanel()
{
    Scene *scene = SceneContext::instance().getMutableScene();
    if (!scene || !scene->camera)
        return;
    ImGui::SetNextWindowPos(ImVec2(10.f, 590.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(340.f, 230.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Camera");
    Vec3 pos = scene->camera->getPosition();
    ImGui::Text("Pos: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
    ImGui::SeparatorText("Move");
    ImGui::SetNextItemWidth(120.f);
    ImGui::SliderFloat("Step##ms", &moveStep_, 0.05f, 5.0f, "%.2f");
    buildCamMoveButtons(*scene->camera);
    ImGui::SeparatorText("Rotate");
    ImGui::SetNextItemWidth(120.f);
    ImGui::SliderFloat("Deg##rd", &rotateDeg_, 1.0f, 45.0f, "%.1f");
    buildCamRotateButtons(*scene->camera);
    ImGui::End();
}
