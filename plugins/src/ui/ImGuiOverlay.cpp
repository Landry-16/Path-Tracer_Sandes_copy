/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** ImGuiOverlay
*/

#include <imgui.h>
#include <imgui-SFML.h>
#include <iostream>

#include "ImGuiOverlay.hpp"
#include "ImGuiOverlayConfig.hpp"
#include "rt/core/UIRegistry.hpp"
#include "rt/core/SceneContext.hpp"

/** @brief Returns true when event is a left-click outside any ImGui window. */
static bool isPickClick(const sf::Event &event, const ImGuiIO &io)
{
    return event.type == sf::Event::MouseButtonPressed
        && event.mouseButton.button == sf::Mouse::Left
        && !io.WantCaptureMouse;
}

/** @brief Routes a non-pick event and returns whether ImGui captured it. */
static bool routeEvent(const sf::Event &event, const ImGuiIO &io)
{
    switch (event.type) {
        case sf::Event::MouseButtonPressed:
        case sf::Event::MouseButtonReleased:
        case sf::Event::MouseMoved:
        case sf::Event::MouseWheelScrolled:
            return io.WantCaptureMouse;
        case sf::Event::KeyPressed:
        case sf::Event::KeyReleased:
        case sf::Event::TextEntered:
            return io.WantCaptureKeyboard;
        default:
            return false;
    }
}

void ImGuiOverlay::init(sf::RenderWindow &window)
{
    window_ = &window;
    if (!ImGui::SFML::Init(window))
        std::cerr << "ImGuiOverlay: ImGui::SFML::Init failed\n";
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    SceneContext::instance().setSceneChangedCallback([this]() {
        sceneChangedFlag_.store(true);
    });
    loadConfig();
}

bool ImGuiOverlay::processEvent(const sf::Event &event)
{
    ImGui::SFML::ProcessEvent(*window_, event);
    const ImGuiIO &io = ImGui::GetIO();
    if (isPickClick(event, io)) {
        tryPick(event.mouseButton.x, event.mouseButton.y);
        return false;
    }
    return routeEvent(event, io);
}

void ImGuiOverlay::update()
{
    ImGui::SFML::Update(*window_, deltaClock_.restart());
    if (!configLoaded_)
        loadConfig();
    if (sceneChangedFlag_.exchange(false))
        handleSceneChange();
    buildUI();
}

void ImGuiOverlay::render(sf::RenderWindow &window)
{
    ImGui::SFML::Render(window);
}

void ImGuiOverlay::shutdown()
{
    SceneContext::instance().setSceneChangedCallback(nullptr);
    ImGui::SFML::Shutdown();
    window_ = nullptr;
}

void ImGuiOverlay::loadConfig()
{
    configLoaded_ = false;
    const std::string &path = SceneContext::instance().getScenePath();
    if (path.empty())
        return;
    try {
        cfg_.readFile(path.c_str());
        configLoaded_ = true;
        dirty_ = false;
    } catch (const libconfig::FileIOException &e) {
        std::cerr << "ImGuiOverlay: cannot open scene file: " << e.what() << "\n";
    } catch (const libconfig::ParseException &e) {
        std::cerr << "ImGuiOverlay: parse error line "
                  << e.getLine() << ": " << e.getError() << "\n";
    }
}

void ImGuiOverlay::saveAndReload()
{
    if (!configLoaded_)
        return;
    const std::string &path = SceneContext::instance().getScenePath();
    if (path.empty())
        return;
    try {
        cfg_.writeFile(path.c_str());
        dirty_ = false;
    } catch (const libconfig::FileIOException &e) {
        std::cerr << "ImGuiOverlay: cannot write scene file: " << e.what() << "\n";
        return;
    }
    SceneContext::instance().triggerReload();
}

void ImGuiOverlay::handleSceneChange()
{
    pick_ = PickResult{};
    loadConfig();
}

void ImGuiOverlay::buildUI()
{
    buildInspectorPanel();
    buildLightsPanel();
    buildCameraPanel();
    buildModals();
}

extern "C" void rt_plugin_register()
{
    UIRegistry::instance().setOverlay(std::make_shared<ImGuiOverlay>());
}
