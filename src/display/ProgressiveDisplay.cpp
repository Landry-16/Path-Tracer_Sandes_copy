/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** ProgressiveDisplay
*/

#include "rt/display/ProgressiveDisplay.hpp"
#include <iostream>

namespace rt {

static const std::unordered_map<sf::Keyboard::Key, std::string> CAMERA_KEYS = {
    {sf::Keyboard::Up, "forward"},
    {sf::Keyboard::Down, "backward"},
    {sf::Keyboard::Left, "left"},
    {sf::Keyboard::Right, "right"},
    {sf::Keyboard::PageUp, "up"},
    {sf::Keyboard::PageDown, "down"},
    {sf::Keyboard::I, "pitch_up"},
    {sf::Keyboard::K, "pitch_down"},
    {sf::Keyboard::L, "yaw_left"},
    {sf::Keyboard::J, "yaw_right"}
};

ProgressiveDisplay::ProgressiveDisplay(int width, int height)
    : width_(width), height_(height),
      window_(sf::VideoMode(width, height), "Ray Tracer - Progressive Render"),
      pixels_(width * height * 4, 0)
{
    texture_.create(width, height);
    sprite_.setTexture(texture_);
    window_.setFramerateLimit(30);
    if (!font_.loadFromFile(std::string(FONT_FILE_PATH)))
        std::cerr << "Warning: Could not load font for status display" << std::endl;
    else
        configureStatusText();
    initKeyHandlers();
    uiOverlay_ = UIRegistry::instance().getOverlay();
    if (uiOverlay_)
        uiOverlay_->init(window_);
}

ProgressiveDisplay::~ProgressiveDisplay()
{
    if (uiOverlay_) {
        uiOverlay_->shutdown();
        uiOverlay_.reset();
    }
    if (window_.isOpen())
        window_.close();
}

void ProgressiveDisplay::configureStatusText()
{
    statusText_.setFont(font_);
    statusText_.setCharacterSize(16);
    statusText_.setFillColor(sf::Color::White);
    statusText_.setOutlineColor(sf::Color::Black);
    statusText_.setOutlineThickness(2);
    statusText_.setPosition(10, 10);
}

void ProgressiveDisplay::initKeyHandlers()
{
    keyHandlers_[sf::Keyboard::Escape] = [this] { closeAndStop(); };
    keyHandlers_[sf::Keyboard::S] = [this] { triggerSave(); };
    keyHandlers_[sf::Keyboard::R] = [this] { triggerReload(); };
}

bool ProgressiveDisplay::inBounds(int x, int y) const
{
    return x >= 0 && x < width_ && y >= 0 && y < height_;
}

void ProgressiveDisplay::writePixel(int index, const Color &color)
{
    pixels_[index + 0] = static_cast<sf::Uint8>(
        std::clamp(color.x, 0.0, 1.0) * 255);
    pixels_[index + 1] = static_cast<sf::Uint8>(
        std::clamp(color.y, 0.0, 1.0) * 255);
    pixels_[index + 2] = static_cast<sf::Uint8>(
        std::clamp(color.z, 0.0, 1.0) * 255);
    pixels_[index + 3] = 255;
}

void ProgressiveDisplay::writeRegionPixel(int px, int py, int colorIdx,
                                          const std::vector<Color> &colors)
{
    if (inBounds(px, py) && colorIdx < static_cast<int>(colors.size()))
        writePixel((py * width_ + px) * 4, colors[colorIdx]);
}

void ProgressiveDisplay::updatePixel(int x, int y, const Color &color)
{
    if (inBounds(x, y)) {
        std::lock_guard<std::mutex> lock(pixelMutex_);
        writePixel((y * width_ + x) * 4, color);
        needsUpdate.store(true);
    }
}

void ProgressiveDisplay::updateRegion(int x, int y, int w, int h,
                                        const std::vector<Color> &colors)
{
    std::lock_guard<std::mutex> lock(pixelMutex_);

    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i)
            writeRegionPixel(x + i, y + j, j * w + i, colors);
    needsUpdate.store(true);
}

bool ProgressiveDisplay::isOpen() const
{
    return window_.isOpen();
}

void ProgressiveDisplay::setStatusText(const std::string &text)
{
    std::lock_guard<std::mutex> lock(statusMutex_);
    statusString_ = text;
    statusText_.setString(text);
}

void ProgressiveDisplay::closeAndStop()
{
    if (uiOverlay_) {
        uiOverlay_->shutdown();
        uiOverlay_.reset();
    }
    window_.close();
    stopRequested.store(true);
}

void ProgressiveDisplay::triggerSave()
{
    if (saveCallback_) {
        std::cout << "Saving image..." << std::endl;
        saveCallback_();
        std::cout << "Image saved!" << std::endl;
    }
}

void ProgressiveDisplay::triggerReload()
{
    if (reloadCallback_) {
        std::cout << "Reloading scene..." << std::endl;
        reloadCallback_();
    }
}

void ProgressiveDisplay::handleCameraKey(sf::Keyboard::Key key)
{
    auto it = CAMERA_KEYS.find(key);

    if (it != CAMERA_KEYS.end() && cameraControlCallback_)
        cameraControlCallback_(it->second);
}

void ProgressiveDisplay::handleKeyPress(sf::Keyboard::Key key)
{
    auto it = keyHandlers_.find(key);

    if (it != keyHandlers_.end())
        it->second();
    else
        handleCameraKey(key);
}

void ProgressiveDisplay::handleEvent(const sf::Event &event)
{
    if (uiOverlay_ && uiOverlay_->processEvent(event))
        return;
    if (event.type == sf::Event::Closed)
        closeAndStop();
    else if (event.type == sf::Event::KeyPressed)
        handleKeyPress(event.key.code);
}

void ProgressiveDisplay::handleEvents()
{
    sf::Event event;

    while (window_.pollEvent(event))
        handleEvent(event);
}

void ProgressiveDisplay::draw()
{
    if (needsUpdate.load()) {
        std::lock_guard<std::mutex> lock(pixelMutex_);
        texture_.update(pixels_.data());
        needsUpdate.store(false);
    }
    if (uiOverlay_)
        uiOverlay_->update();
    window_.clear(sf::Color::Black);
    window_.draw(sprite_);
    if (font_.getInfo().family != "") {
        std::lock_guard<std::mutex> lock(statusMutex_);
        window_.draw(statusText_);
    }
    if (uiOverlay_)
        uiOverlay_->render(window_);
    window_.display();
}

void ProgressiveDisplay::eventLoop()
{
    while (window_.isOpen()) {
        handleEvents();
        if (!window_.isOpen())
            break;
        draw();
        sf::sleep(sf::milliseconds(16));
    }
}

void ProgressiveDisplay::run()
{
    eventLoop();
}

void ProgressiveDisplay::waitForClose()
{
    eventLoop();
}

}
