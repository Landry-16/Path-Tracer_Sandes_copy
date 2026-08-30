/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** OBJMesh
*/

#ifndef OBJMESH_HPP
#define OBJMESH_HPP
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <filesystem>
#include <functional>
#include <sstream>
#include "rt/interfaces/IPrimitive.hpp"
#include "rt/interfaces/IMaterial.hpp"
#include "rt/interfaces/ITexture.hpp"
#include "rt/math/Vec3.hpp"
#include "rt/rendering/BVH.hpp"
#include "rt/math/Matrix4.hpp"
#include "rt/math/Color.hpp"

struct Vec2 {
    double x, y;
    Vec2() : x(0), y(0) {}
    Vec2(double x, double y) : x(x), y(y) {}
    Vec2 operator*(double s) const { return Vec2(x * s, y * s); }
    Vec2 operator+(const Vec2 &v) const { return Vec2(x + v.x, y + v.y); }
};

struct MTLMaterial {
    std::string name;
    Color ambient;
    Color diffuse;
    Color specular;
    double shininess;
    std::string diffuseTexturePath;
    std::string normalMapPath;
    std::shared_ptr<ITexture> diffuseTexture;
    std::shared_ptr<IMaterial> material;
    
    MTLMaterial() : ambient(0.2, 0.2, 0.2), diffuse(0.8, 0.8, 0.8), 
                    specular(1.0, 1.0, 1.0), shininess(32.0), material(nullptr) {}
};

class Triangle : public IPrimitive {
public:
    Vec3 v0, v1, v2;
    Vec3 normal;  // Flat normal (computed from geometry)
    std::shared_ptr<const IMaterial> material;
    Vec3 n0, n1, n2;  // Smooth normals (per-vertex)
    Vec2 uv0, uv1, uv2;
    bool hasVertexNormals;
    AABB bounds;
    
    Triangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2, std::shared_ptr<const IMaterial> mat);
    Triangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2, 
             const Vec2 &uv0, const Vec2 &uv1, const Vec2 &uv2, std::shared_ptr<const IMaterial> mat);
    Triangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2,
             const Vec3 &n0, const Vec3 &n1, const Vec3 &n2,
             const Vec2 &uv0, const Vec2 &uv1, const Vec2 &uv2, std::shared_ptr<const IMaterial> mat);
    
    bool intersect(const Ray &ray, double tMin, double tMax, HitRecord &hit) const override;
    bool intersectShadow(const Ray &ray, double tMin, double tMax) const override;
    AABB getBounds() const override;
};

class OBJMesh : public IPrimitive {
public:
    OBJMesh(const std::string &filepath, std::shared_ptr<const IMaterial> material, const Matrix4 &transform);
    ~OBJMesh();
    
    bool intersect(const Ray &ray, double tMin, double tMax, HitRecord &hit) const override;
    bool intersectShadow(const Ray &ray, double tMin, double tMax) const override;
    AABB getBounds() const override;
    
private:
    std::vector<std::unique_ptr<IPrimitive>> triangles;
    std::unique_ptr<BVH> bvh;
    AABB bounds;
    Matrix4 transform;
    Matrix4 invTransform;
    std::shared_ptr<const IMaterial> defaultMaterial;
    Color fallbackColor;
    std::shared_ptr<IMaterial> fallbackMaterial;
    
    std::map<std::string, MTLMaterial> materials;
    std::vector<std::shared_ptr<IMaterial>> ownedMaterials;
    std::string objDirectory;
    std::filesystem::path objDirectoryPath;
    
    bool loadOBJ(const std::string &filepath);
    bool loadMTL(const std::string &mtlPath);
    std::string resolveTexturePath(const std::string &texPath, bool mustExist = false) const;
    std::string findFallbackDiffuseTexture(const std::string &materialName) const;
    static std::string trim(const std::string &value);
    void applyTransformations();
    void applyMaterialOverride(std::shared_ptr<const IMaterial> material);
    void assignMissingMaterials();
    std::shared_ptr<const IMaterial> getFallbackMaterial();
    void computeBounds();
    void buildBVH();

    // OBJ parsing helpers
    void parseGeometryLine(const std::string &type, std::istringstream &iss,
                           std::vector<Vec3> &vertices,
                           std::vector<Vec2> &texcoords,
                           std::vector<Vec3> &normals);
    void parseMaterialLine(const std::string &type, std::istringstream &iss,
                           std::string &currentMaterialName,
                           std::shared_ptr<const IMaterial> &currentMaterial);
    void parseFaceIndices(std::istringstream &iss,
                          std::vector<int> &vi,
                          std::vector<int> &vti,
                          std::vector<int> &vni);
    std::unique_ptr<Triangle> buildFaceTriangle(
        const std::vector<Vec3> &verts, const std::vector<Vec2> &tcs,
        const std::vector<Vec3> &norms,
        const std::vector<int> &vi, const std::vector<int> &vti,
        const std::vector<int> &vni, size_t i,
        bool hasNorm, bool hasTex,
        std::shared_ptr<const IMaterial> mat) const;
    void addFaceTriangles(const std::vector<Vec3> &verts,
                          const std::vector<Vec2> &tcs,
                          const std::vector<Vec3> &norms,
                          const std::vector<int> &vi,
                          const std::vector<int> &vti,
                          const std::vector<int> &vni,
                          std::shared_ptr<const IMaterial> mat);
    void parseFaceLine(std::istringstream &iss,
                       const std::vector<Vec3> &verts,
                       const std::vector<Vec2> &tcs,
                       const std::vector<Vec3> &norms,
                       std::shared_ptr<const IMaterial> mat);

    // MTL parsing helpers
    void parseMTLColorOrFloat(const std::string &type, std::istringstream &iss,
                               MTLMaterial &mtl);
    void parseMTLTextureProp(const std::string &type, std::istringstream &iss,
                              MTLMaterial &mtl,
                              const std::function<std::string(std::istringstream&)> &parseTex);
    void parseMTLToken(const std::string &type, std::istringstream &iss,
                       MTLMaterial*& current,
                       const std::function<std::string(std::istringstream&)> &parseTex);
    void tryLoadTexturedMaterial(MTLMaterial &mtl);
    void finalizeMTLMaterial(MTLMaterial &mtl, const std::string &name);

    // Texture resolution helpers
    std::vector<std::filesystem::path> buildSearchRoots() const;
    std::string searchRelativeInRoots(const std::filesystem::path &rel,
                                      const std::vector<std::filesystem::path> &roots) const;
    std::string tryExtensionVariants(const std::filesystem::path &filename,
                                     const std::vector<std::filesystem::path> &roots) const;
    std::vector<std::string> buildMaterialNameVariants(const std::string &name) const;
};

#endif // OBJMESH_HPP
