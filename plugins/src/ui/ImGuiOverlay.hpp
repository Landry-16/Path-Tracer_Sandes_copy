/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** ImGuiOverlay
*/

#ifndef IMGUIOVERLAY_HPP
    #define IMGUIOVERLAY_HPP

    #include <SFML/Graphics.hpp>
    #include <libconfig.h++>
    #include <string>
    #include <atomic>
    #include <memory>

    #include "rt/interfaces/IUIOverlay.hpp"
    #include "rt/scene/Scene.hpp"

/** @brief Stores the result of a ray-scene intersection used for object picking. */
struct PickResult {
    int primitiveIndex = -1;
    Vec3 hitPoint;
    Vec3 hitNormal;
    double hitT = 0.0;
    std::shared_ptr<const IMaterial> material;
};

/** @brief ImGui-based overlay implementing the full scene editor with libconfig persistence. */
class ImGuiOverlay : public IUIOverlay {
public:
    void init(sf::RenderWindow &window) override;
    bool processEvent(const sf::Event &event) override;
    void update() override;
    void render(sf::RenderWindow &window) override;
    void shutdown() override;

private:
    sf::RenderWindow *window_ = nullptr;
    sf::Clock deltaClock_;
    PickResult pick_;
    std::atomic<bool> sceneChangedFlag_{false};

    libconfig::Config cfg_;
    bool configLoaded_ = false;
    bool dirty_ = false;

    float editColor_[3] = {1, 1, 1};
    float editEmission_[3] = {0, 0, 0};
    float editReflectivity_ = 0.f;
    float editRoughness_ = 0.5f;
    float editMetallic_ = 0.f;
    float editIOR_ = 1.5f;

    float editCenter_[3] = {0, 0, 0};
    float editRadius_ = 1.f;
    float editPoint_[3] = {0, 0, 0};
    float editNormal_[3] = {0, 1, 0};

    float moveStep_ = 0.3f;
    float rotateDeg_ = 5.0f;

    bool openAddPrimModal_ = false;
    int newPrimType_ = 0;
    float newPrimPos_[3] = {0, 1, 0};
    float newPrimNormal_[3] = {0, 1, 0};
    float newPrimRadius_ = 1.f;
    float newPrimColor_[3] = {1, 1, 1};
    int newPrimMatType_ = 0;
    float newPrimReflectivity_ = 0.8f;
    float newPrimTorusDir_[3]  = {1, 0, 0};
    float newPrimTorusRot_[3]  = {0, 0, 0};

    bool openAddLightModal_ = false;
    int newLightType_ = 0;
    float newLightColor_[3] = {1, 1, 1};
    float newLightIntensity_ = 0.5f;
    float newLightDir_[3] = {0, -1, 0};
    float newLightPos_[3] = {0,  5, 0};

    void loadConfig();
    void saveAndReload();
    void handleSceneChange();

    libconfig::Setting *getObject(int idx);
    libconfig::Setting *getLight(int idx);
    std::string getObjType(int idx);
    std::string getLightType(int idx);
    std::string getMatType(int idx);

    void tryPick(int mouseX, int mouseY);
    void applyPickResult(int idx, const HitRecord &hit);
    void syncEditBuffers();
    void syncEditFromMaterial(const IMaterial &mat);
    void syncEditFromConfig(libconfig::Setting &obj);
    void syncGeometryBuffers();
    void syncPlaneBuffers(libconfig::Setting &obj);
    void selectPrimitive(int idx);

    void removePrimitive(int idx);
    void addPrimGeometry(libconfig::Setting &obj);
    void addPrimMaterial(libconfig::Setting &obj);
    void addPrimitive();
    void removeLight(int idx);
    void addLight();

    void buildUI();
    void buildInspectorPanel();
    void buildInspectorHeader(const Scene *scene);
    void buildObjectList();
    void buildObjectEditor(const Scene *scene);
    bool buildPrimitiveHeader(int idx);
    void buildGeometryEditor(int idx);
    void applySphereGeo(libconfig::Setting &obj);
    void applyPlaneGeo(libconfig::Setting &obj);
    void buildMaterialEditor(int idx);
    void initMatDefaults(libconfig::Setting &obj, const char *matName);
    bool buildMatTypeCombo(
        libconfig::Setting *obj, const std::string &mt,
        const char **matTypes, int nMat);
    bool buildMatSliders(const std::string &mt);
    void pushMatToLive(IMaterial *mmat);
    void pushMatToConfig(libconfig::Setting &obj, const std::string &mt);

    void buildLightsPanel();
    void readLightState(int i, ILight *light, float col[3], float &intensity);
    void applyLightColor(
        int i, ILight *light, const float col[3], float intensity);
    void buildLightPosFields(int i, const std::string &kind);
    bool buildLightEntry(int i);

    void buildCameraPanel();
    void buildCamMoveButtons(Camera &cam);
    void buildCamRotateButtons(Camera &cam);

    void buildModals();
    void buildNewPrimGeoFields();
    void buildAddPrimitiveModal();
    void buildAddLightModal();
};

#endif // IMGUIOVERLAY_HPP
