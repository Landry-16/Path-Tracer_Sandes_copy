/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** pbr_material
*/

#include <map>
#include <memory>
#include <cmath>
#include <algorithm>
#include <random>
#include "rt/interfaces/IMaterial.hpp"
#include "rt/interfaces/ITexture.hpp"
#include "rt/textures/SolidColorTexture.hpp"
#include "rt/core/Factory.hpp"
#include "rt/math/Color.hpp"
#include "rt/math/Vec3.hpp"
#include "BRDFFrame.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class PBRMaterial : public IMaterial {
public:
    PBRMaterial(const Color &albedo,
                double metallic,
                double roughness,
                const Color &emission,
                std::shared_ptr<ITexture> texture = nullptr)
        : albedo(albedo)
        , metallic(std::clamp(metallic, 0.0, 1.0))
        , roughness(std::clamp(roughness, 0.03, 1.0))
        , emission(emission)
        , albedoTexture(texture ? std::move(texture) : std::make_shared<SolidColorTexture>(albedo))
    {}

    Color getColor() const override
    {
        return albedo;
    }

    Color getDiffuse() const override
    {
        return albedo * (1.0 - metallic);
    }

    bool isReflective() const override
    {
        return roughness < 0.5 || metallic > 0.5;
    }

    double getReflectivity() const override
    {
        return metallic;
    }

    std::shared_ptr<const ITexture> getTexture() const override
    {
        return albedoTexture;
    }

    bool isPBR() const override
    {
        return true;
    }

    double getMetallic() const override
    {
        return metallic;
    }

    double getRoughness() const override
    {
        return roughness;
    }

    Color getEmission() const override
    {
        return emission;
    }

    void setColor(const Color &c) override { albedo = c; }
    void setRoughness(double r) override { roughness = std::clamp(r, 0.03, 1.0); }
    void setMetallic(double m) override { metallic = std::clamp(m, 0.0, 1.0); }
    void setEmission(const Color &e) override { emission = e; }

    Color getAlbedo(double u, double v) const
    {
        if (albedoTexture) {
            return albedoTexture->sample(u, v);
        }
        return albedo;
    }

    Color getF0() const
    {
        Color dielectric(0.04, 0.04, 0.04);
        return dielectric * (1.0 - metallic) + albedo * metallic;
    }

    static double distributionGGX(const Vec3 &N, const Vec3 &H, double roughness)
    {
        double a2 = std::pow(roughness, 4.0);
        double NdH = std::max(N.dot(H), 0.0);
        double denom = (NdH * NdH * (a2 - 1.0) + 1.0);
        return a2 / std::max(M_PI * denom * denom, 1e-7);
    }

    static double geometrySchlickGGX(double NdotV, double roughness)
    {
        double r = roughness + 1.0;
        double k = (r * r) / 8.0;
        double denom = NdotV * (1.0 - k) + k;
        return NdotV / std::max(denom, 0.0000001);
    }

    static double geometrySmith(const Vec3 &N, const Vec3 &V, const Vec3 &L, double roughness)
    {
        double NdotV = std::max(N.dot(V), 0.0);
        double NdotL = std::max(N.dot(L), 0.0);
        return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
    }

    static Color fresnelSchlick(double cosTheta, const Color &F0)
    {
        double f = std::pow(std::clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
        return F0 * (1.0 - f) + Color(1.0, 1.0, 1.0) * f;
    }

    Color evaluateBRDF(const Vec3 &wo, const Vec3 &wi, const Vec3 &normal,
                       double u, double v) const override
    {
        BRDFFrame f = BRDFFrame::make(wo, wi, normal);
        if (f.NdotL < 1e-6 || f.NdotV < 1e-6) return Color(0, 0, 0);
        Color spec = cookTorranceSpecular(f);
        Color F = fresnelSchlick(std::max(f.H.dot(f.V), 0.0), getF0());
        Color kd = (Color(1, 1, 1) - F) * (1.0 - metallic);
        return kd * getAlbedo(u, v) * (1.0 / M_PI) + spec;
    }

    static void buildOrthonormalBasis(const Vec3 &normal, Vec3 &tangent, Vec3 &bitangent)
    {
        Vec3 up = std::abs(normal.y) < 0.999 ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
        tangent = up.cross(normal).normalized();
        bitangent = normal.cross(tangent);
    }

    static Vec3 sampleCosineHemisphere(const Vec3 &N, double r1, double r2)
    {
        double phi = 2.0 * M_PI * r1;
        double cosTheta = std::sqrt(1.0 - r2);
        double sinTheta = std::sqrt(r2);

        Vec3 local(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);

        Vec3 tangent, bitangent;
        buildOrthonormalBasis(N, tangent, bitangent);

        return (tangent * local.x + bitangent * local.y + N * local.z).normalized();
    }

    static Vec3 sampleMicrofacetH(const Vec3 &N, double a2, double r1, double r2)
    {
        double phi = 2.0 * M_PI * r1;
        double cosTheta = std::sqrt((1.0 - r2) / (1.0 + (a2 - 1.0) * r2));
        double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);
        Vec3 tangent, bitangent;
        buildOrthonormalBasis(N, tangent, bitangent);
        Vec3 lH(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);
        return (tangent * lH.x + bitangent * lH.y + N * lH.z).normalized();
    }

    static Vec3 sampleGGX(const Vec3 &N, const Vec3 &V, double roughness, double r1, double r2)
    {
        double a2 = std::pow(roughness, 4.0);
        Vec3 H = sampleMicrofacetH(N, a2, r1, r2);
        return (H * 2.0 * std::max(0.0, V.dot(H)) - V).normalized();
    }

    static double pdfGGX(const Vec3 &N, const Vec3 &H, const Vec3 &wo, const Vec3&, double roughness)
    {
        double a = roughness * roughness;
        double a2 = a * a;
        double NdotH = std::max(N.dot(H), 0.0);
        double NdotH2 = NdotH * NdotH;

        double denom = (NdotH2 * (a2 - 1.0) + 1.0);
        double D = a2 / (M_PI * denom * denom);

        double HdotWo = std::max(H.dot(wo), 0.0);
        double pdf = (D * NdotH) / (4.0 * HdotWo);

        return std::max(pdf, 0.0000001);
    }

    ScatterResult scatter(const Vec3 &wo, const Vec3 &normal,
                         double, double,
                         double random1, double random2) const override
    {
        Vec3 N = normal.normalized();
        Vec3 V = wo.normalized();
        double sw = computeSpecWeight(N, V);
        if (sw < 0.0) return ScatterResult{};
        double choice = std::fmod(random1 * 12.9898 + random2 * 78.233, 1.0);
        Vec3 wi = (choice < sw || metallic > 0.9)
            ? sampleGGX(N, V, roughness, random1, random2)
            : sampleCosineHemisphere(N, random1, random2);
        return buildFromWi(wo, N, wi, sw);
    }

private:
    Color albedo;
    double metallic;
    double roughness;
    Color emission;
    std::shared_ptr<ITexture> albedoTexture;

    Color cookTorranceSpecular(const BRDFFrame &f) const
    {
        double D = distributionGGX(f.N, f.H, roughness);
        double G = geometrySmith(f.N, f.V, f.L, roughness);
        Color F = fresnelSchlick(std::max(f.H.dot(f.V), 0.0), getF0());
        return F * (D * G / std::max(4.0 * f.NdotV * f.NdotL, 1e-7));
    }

    double computeSpecWeight(const Vec3 &N, const Vec3 &V) const
    {
        Color F = fresnelSchlick(std::max(N.dot(V), 0.0), getF0());
        double sw = (F.x + F.y + F.z) / 3.0;
        double total = sw + (1.0 - sw) * (1.0 - metallic);
        return (total < 0.0001) ? -1.0 : sw / total;
    }

    ScatterResult buildFromWi(const Vec3 &wo, const Vec3 &N, const Vec3 &wi, double sw) const
    {
        ScatterResult r; r.valid = false;
        if (wi.dot(N) <= 0.0) return r;
        Vec3 V = wo.normalized();
        Vec3 H = (V + wi).normalized();
        double pS = pdfGGX(N, H, V, wi, roughness);
        double pD = std::max(wi.dot(N), 0.0) / M_PI;
        double pdf = sw * pS + (1.0 - sw) * pD;
        if (pdf < 1e-6) return r;
        r.direction = wi;
        r.valid = true;
        r.pdf = pdf;
        r.attenuation = evaluateBRDF(wo, wi, N, 0, 0) * std::max(0.0, wi.dot(N)) / pdf;
        return r;
    }
};

namespace {
    struct FixedPreset { Color color; double metallic; double roughness; };
    static const std::map<std::string, FixedPreset> FIXED_PRESETS = {
        {"pbr_gold",        {{1.0,    0.765557, 0.336057}, 1.0, 0.10}},
        {"pbr_silver",      {{0.9715, 0.9599,   0.9153},   1.0, 0.05}},
        {"pbr_copper",      {{0.9550, 0.6374,   0.5382},   1.0, 0.15}},
        {"pbr_aluminum",    {{0.9132, 0.9215,   0.9245},   1.0, 0.20}},
        {"pbr_iron",        {{0.560,  0.570,    0.580},    1.0, 0.40}},
        {"pbr_chrome",      {{0.5496, 0.5561,   0.5543},   1.0, 0.02}},
        {"pbr_jade",        {{0.05,   0.3,      0.2},      0.0, 0.25}},
        {"pbr_marble",      {{0.9,    0.9,      0.88},     0.0, 0.12}},
        {"pbr_wood",        {{0.4,    0.25,     0.15},     0.0, 0.70}},
        {"pbr_glass_rough", {{1.0,    1.0,      1.0},      0.0, 0.02}},
    };

    struct ParamPreset { double metallic; double roughness; };
    static const std::map<std::string, ParamPreset> PARAM_PRESETS = {
        {"pbr_plastic",        {0.0, 0.30}},
        {"pbr_glossy_plastic", {0.0, 0.05}},
        {"pbr_rubber",         {0.0, 0.80}},
        {"pbr_ceramic",        {0.0, 0.15}},
        {"pbr_rough_metal",    {1.0, 0.60}},
        {"pbr_polished_metal", {1.0, 0.05}},
    };

    std::shared_ptr<ITexture> createTextureFromParams(const MaterialParams &params)
    {
        if (params.hasTexture && !params.textureType.empty() && TextureRegistry::instance().isRegistered(params.textureType)) {
            return TextureRegistry::instance().create(params.textureType, params.textureParams);
        }
        return nullptr;
    }

    std::unique_ptr<IMaterial> makePBR(const Color &color, double metallic, double roughness,
                                        const Color &emission, const MaterialParams &params)
    {
        auto texture = createTextureFromParams(params);
        return std::make_unique<PBRMaterial>(color, metallic, roughness, emission, std::move(texture));
    }

    void registerFixedPresets()
    {
        for (const auto &entry : FIXED_PRESETS) {
            const FixedPreset fp = entry.second;
            MaterialRegistry::instance().registerType(entry.first,
                [fp](const MaterialParams &p) -> std::unique_ptr<IMaterial> {
                    return makePBR(fp.color, fp.metallic, fp.roughness, Color(0,0,0), p);
                }
            );
        }
    }

    void registerParamPresets()
    {
        for (const auto &entry : PARAM_PRESETS) {
            const ParamPreset pp = entry.second;
            MaterialRegistry::instance().registerType(entry.first,
                [pp](const MaterialParams &p) -> std::unique_ptr<IMaterial> {
                    return makePBR(p.color, pp.metallic, pp.roughness, Color(0,0,0), p);
                }
            );
        }
    }
}

extern "C" {
    void rt_plugin_register()
    {
        MaterialRegistry::instance().registerType("pbr",
            [](const MaterialParams &p) -> std::unique_ptr<IMaterial> {
                return makePBR(p.color, std::clamp(p.reflectivity, 0.0, 1.0),
                               std::clamp(p.roughness, 0.03, 1.0), Color(0,0,0), p);
            }
        );
        MaterialRegistry::instance().registerType("pbr_emissive",
            [](const MaterialParams &p) -> std::unique_ptr<IMaterial> {
                return makePBR(Color(0,0,0), 0.0, 1.0, p.color * 10.0, p);
            }
        );
        registerFixedPresets();
        registerParamPresets();
    }
}
