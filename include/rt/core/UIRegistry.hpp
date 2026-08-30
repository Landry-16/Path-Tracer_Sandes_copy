/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** UIRegistry
*/

#ifndef RT_UIREGISTRY_HPP
    #define RT_UIREGISTRY_HPP

    #include <memory>
    #include "rt/interfaces/IUIOverlay.hpp"

/**
 * Global registry for the active UI overlay.
 * A plugin's rt_plugin_register() calls:
 *   UIRegistry::instance().setOverlay(std::make_shared<MyOverlay>());
 *
 * ProgressiveDisplay reads the overlay once at construction and calls its
 * lifecycle methods. Only one overlay can be active at a time.
 */
class UIRegistry {
public:
    static UIRegistry &instance() {
        static UIRegistry reg;
        return reg;
    }

    void setOverlay(std::shared_ptr<IUIOverlay> overlay) {
        overlay_ = std::move(overlay);
    }

    std::shared_ptr<IUIOverlay> getOverlay() const {
        return overlay_;
    }

    bool hasOverlay() const {
        return overlay_ != nullptr;
    }

private:
    UIRegistry() = default;
    UIRegistry(const UIRegistry &) = delete;
    UIRegistry &operator=(const UIRegistry &) = delete;

    std::shared_ptr<IUIOverlay> overlay_;
};

#endif // RT_UIREGISTRY_HPP
