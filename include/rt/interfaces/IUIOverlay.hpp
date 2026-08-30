/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** IUIOverlay
*/

#ifndef RT_IUIOVERLAY_HPP
    #define RT_IUIOVERLAY_HPP

    #include <SFML/Graphics.hpp>

class IUIOverlay {
public:
    virtual ~IUIOverlay() = default;

    virtual void init(sf::RenderWindow &window) = 0;

    /** Process a raw SFML event.
     * @return true if the event was consumed and should not be processed further.
    */
    virtual bool processEvent(const sf::Event &event) = 0;

    /* Called once per frame before render(). Update internal ImGui state here. */
    virtual void update() = 0;

    /* Draw ImGui widgets. Called after the scene sprite, before window.display(). */
    virtual void render(sf::RenderWindow &window) = 0;

    /* Called when the window is about to close. Release ImGui resources here. */
    virtual void shutdown() = 0;
};

#endif // RT_IUIOVERLAY_HPP
