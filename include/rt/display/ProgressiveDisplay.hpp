/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** ProgressiveDisplay
*/

#ifndef RT_PROGRESSIVEDISPLAY_HPP
    #define RT_PROGRESSIVEDISPLAY_HPP

    #include <SFML/Graphics.hpp>
    #include <atomic>
    #include <mutex>
    #include <vector>
    #include <string>
    #include <functional>
    #include <unordered_map>
    #include <memory>
    #include "rt/math/Color.hpp"
    #include "rt/core/UIRegistry.hpp"

static constexpr std::string_view FONT_FILE_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";

namespace rt {

class ProgressiveDisplay {
public:
    ProgressiveDisplay(int width, int height);
    ~ProgressiveDisplay();

    void updatePixel(int x, int y, const Color &color);
    void updateRegion(int x, int y, int width, int height,
        const std::vector<Color> &colors);

    bool isOpen() const;
    bool shouldStop() const { return stopRequested.load(); }
    void requestStop() { stopRequested.store(true); }

    void setSaveCallback(std::function<void()> callback)
        { saveCallback_ = callback; }
    void setReloadCallback(std::function<void()> callback)
        { reloadCallback_ = callback; }
    void setCameraControlCallback(std::function<void(
        const std::string&)> callback) { cameraControlCallback_ = callback; }
    void setStatusText(const std::string &text);

    void run();
    void waitForClose();

private:
    void handleEvents();
    void handleEvent(const sf::Event &event);
    void handleKeyPress(sf::Keyboard::Key key);
    void handleCameraKey(sf::Keyboard::Key key);
    void closeAndStop();
    void triggerSave();
    void triggerReload();
    void initKeyHandlers();
    void configureStatusText();
    void eventLoop();
    void draw();

    bool inBounds(int x, int y) const;
    void writePixel(int index, const Color &color);
    void writeRegionPixel(int px, int py, int colorIdx,
        const std::vector<Color> &colors);

    std::unordered_map<sf::Keyboard::Key, std::function<void()>> keyHandlers_;

    int width_;
    int height_;
    sf::RenderWindow window_;
    sf::Texture texture_;
    sf::Sprite sprite_;
    std::vector<sf::Uint8> pixels_;

    sf::Font font_;
    sf::Text statusText_;
    std::string statusString_;
    std::mutex statusMutex_;

    std::mutex pixelMutex_;
    std::atomic<bool> stopRequested{false};
    std::atomic<bool> needsUpdate{false};

    std::function<void()> saveCallback_;
    std::function<void()> reloadCallback_;
    std::function<void(const std::string&)> cameraControlCallback_;

    std::shared_ptr<IUIOverlay> uiOverlay_;
};

}

#endif // RT_PROGRESSIVEDISPLAY_HPP
