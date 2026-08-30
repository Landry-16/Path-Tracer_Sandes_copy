/*
** EPITECH PROJECT, 2026
** local-Raytracer
** File description:
** STLMesh
*/

#ifndef STLMESH_HPP_
    #define STLMESH_HPP_
    #include <vector>
    #include <string>
    #include <memory>
    #include <cstdint>
    #include <fstream>
    #include "rt/interfaces/IPrimitive.hpp"
    #include "rt/rendering/BVH.hpp"
    #include "rt/math/Matrix4.hpp"
    #include "rt/math/Color.hpp"
    #include "rt/core/Factory.hpp"

inline constexpr std::size_t STL_HEADER_SIZE = 80;
inline constexpr std::size_t STL_TRIANGLE_BYTES = 50;

struct STLVec3 {
    float x;
    float y;
    float z;
};
static_assert(sizeof(STLVec3) == 12, "STLVec3 must be 12 bytes (no padding)");

struct STLLoadStats {
    std::size_t triangles = 0;
    std::size_t degenerate = 0;
    std::size_t coloredTriangles = 0;
    bool hasMaterialiseColor = false;
    Color materialiseColor{1.0, 1.0, 1.0};
    bool hasMaterialiseMaterial = false;
    Color materialiseDiffuse{1.0, 1.0, 1.0};
    Color materialiseSpecular{0.0, 0.0, 0.0};
};

class STLTriangle : public IPrimitive {
public:
    STLTriangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2,
        std::shared_ptr<const IMaterial> mat);
    STLTriangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2,
        const Vec3 &fileNormal, std::shared_ptr<const IMaterial> mat);

    bool intersect(const Ray &ray, double tMin, double tMax,
        HitRecord &hit) const override;
    bool intersectShadow(const Ray &ray, double tMin,
        double tMax) const override;
    AABB getBounds() const override;

    bool isDegenerate() const;

private:
    Vec3 _v0, _v1, _v2;
    Vec3 _normal;
    std::shared_ptr<const IMaterial> _material;
    AABB _bounds;

    void computeBounds();
};

class STLMesh : public IPrimitive {
public:
    STLMesh(const std::string &filepath,
        std::shared_ptr<const IMaterial> material,
        const Matrix4 &transform,
        bool useFileHeader = false,
        const std::string &materialType = "",
        const MaterialParams &materialParams = MaterialParams{});
    ~STLMesh() override = default;

    bool intersect(const Ray &ray, double tMin, double tMax,
        HitRecord &hit) const override;
    bool intersectShadow(const Ray &ray, double tMin,
        double tMax) const override;
    AABB getBounds() const override;

    const STLLoadStats &stats() const { return _stats; }

private:
    std::vector<std::unique_ptr<IPrimitive>> _triangles;
    std::shared_ptr<const IMaterial> _material;
    std::unique_ptr<BVH> _bvh;
    AABB _bounds;
    Matrix4 _transform;
    Matrix4 _invTransform;
    STLLoadStats _stats;
    bool _useFileHeader;
    std::string _materialType;
    MaterialParams _materialParams;

    struct RawTriangle {
        Vec3 v0, v1, v2;
        Vec3 fileNormal;
        bool hasFileNormal;
    };

    bool loadSTL(const std::string &filepath);
    bool loadBinary(std::ifstream &file, std::uintmax_t fileSize,
        std::vector<RawTriangle> &raws);
    void buildTriangles(const std::vector<RawTriangle> &raws);
    void computeBounds();
    void buildBVH();

    static bool isAsciiSTL(std::ifstream &file, std::uintmax_t fileSize);
    static void decodeAttributeColor(std::uint16_t attr,
        bool &valid, Color &out);
    static void decodeMateraliseHeader(const char *header,
        bool &found, Color &out);
    static void decodeMateraliseMaterial(const char *header,
        bool &found, Color &diffuse, Color &specular);
    void resolveHeaderMaterial();

    template<typename T>
    static void readBinary(std::ifstream &file, T &value)
    {
        file.read(reinterpret_cast<char *>(&value), sizeof(T));
    }
};

#endif /* !STLMESH_HPP_ */
