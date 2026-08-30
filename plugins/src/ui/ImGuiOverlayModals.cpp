/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** ImGuiOverlay Add Primitive and Add Light modal dialogs
*/

#include <imgui.h>

#include "ImGuiOverlay.hpp"

void ImGuiOverlay::buildModals()
{
    if (openAddPrimModal_) {
        ImGui::OpenPopup("Add Primitive");
        openAddPrimModal_ = false;
    }
    if (openAddLightModal_) {
        ImGui::OpenPopup("Add Light");
        openAddLightModal_ = false;
    }
    buildAddPrimitiveModal();
    buildAddLightModal();
}

void ImGuiOverlay::buildNewPrimGeoFields()
{
    if (newPrimType_ == 1) {
        ImGui::DragFloat3("Point##nppos", newPrimPos_, 0.1f);
        ImGui::DragFloat3("Normal##npnrm", newPrimNormal_, 0.01f, -1.f, 1.f);
        return;
    }
    ImGui::DragFloat3("Center##nppos", newPrimPos_, 0.1f);
    ImGui::DragFloat("Radius##npr", &newPrimRadius_, 0.05f, 0.001f, 500.f);
    if (newPrimType_ == 7) {
        ImGui::DragFloat3("Direction##nptd", newPrimTorusDir_, 0.01f, -1.f, 1.f);
        ImGui::DragFloat3("Rotation##nptr", newPrimTorusRot_, 1.f, -360.f, 360.f);
    }
}

void ImGuiOverlay::buildAddPrimitiveModal()
{
    if (!ImGui::BeginPopupModal(
        "Add Primitive", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        return;
    static const char *primTypes[] = {
        "Sphere", "Plane", "Box", "Cylinder",
        "Cone", "Pyramid", "Triangle", "Torus"
    };
    ImGui::Combo("Type##npm", &newPrimType_, primTypes, 8);
    buildNewPrimGeoFields();
    ImGui::ColorEdit3("Color##npc", newPrimColor_);
    static const char *mats[] = {"flat", "reflective"};
    ImGui::Combo("Material##npmtype", &newPrimMatType_, mats, 2);
    if (newPrimMatType_ == 1)
        ImGui::SliderFloat("Reflectivity##npref", &newPrimReflectivity_, 0.f, 1.f);
    ImGui::Separator();
    if (ImGui::Button("Create##npc")) {
        addPrimitive();
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel##npc"))
        ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}

void ImGuiOverlay::buildAddLightModal()
{
    if (!ImGui::BeginPopupModal(
        "Add Light", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        return;
    static const char *lTypes[] = {"Ambient", "Directional", "Point"};
    ImGui::Combo("Type##nlm", &newLightType_, lTypes, 3);
    ImGui::ColorEdit3("Color##nlc", newLightColor_);
    ImGui::SliderFloat("Intensity##nli", &newLightIntensity_, 0.f, 5.f);
    if (newLightType_ == 1)
        ImGui::DragFloat3("Direction##nld", newLightDir_, 0.01f, -1.f, 1.f);
    if (newLightType_ == 2)
        ImGui::DragFloat3("Position##nlp", newLightPos_, 0.1f);
    ImGui::Separator();
    if (ImGui::Button("Create##nlcr")) {
        addLight();
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel##nlcc"))
        ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}
