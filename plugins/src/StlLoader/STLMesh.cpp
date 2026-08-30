/*
** EPITECH PROJECT, 2026
** local-Raytracer
** File description:
** stl mesh
*/

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include "StlLoader/STLMesh.hpp"
#include "rt/core/Factory.hpp"

namespace {

constexpr double EPSILON = 1e-8;

AABB transformAABB(const AABB &box, const Matrix4 &m)
{
    Vec3 corners[8] = {
        Vec3(box.min.x, box.min.y, box.min.z),
        Vec3(box.max.x, box.min.y, box.min.z),
        Vec3(box.min.x, box.max.y, box.min.z),
        Vec3(box.max.x, box.max.y, box.min.z),
        Vec3(box.min.x, box.min.y, box.max.z),
        Vec3(box.max.x, box.min.y, box.max.z),
        Vec3(box.min.x, box.max.y, box.max.z),
        Vec3(box.max.x, box.max.y, box.max.z),
    };
    const double INF = std::numeric_limits<double>::max();
    Vec3 lo(INF, INF, INF);
    Vec3 hi(-INF, -INF, -INF);
    for (const Vec3 &c : corners) {
        Vec3 t = m.transformPoint(c);
        lo.x = std::min(lo.x, t.x);
        lo.y = std::min(lo.y, t.y);
        lo.z = std::min(lo.z, t.z);
        hi.x = std::max(hi.x, t.x);
        hi.y = std::max(hi.y, t.y);
        hi.z = std::max(hi.z, t.z);
    }
    return AABB(lo, hi);
}

}

STLMesh::STLMesh(const std::string &filepath,
    std::shared_ptr<const IMaterial> material,
    const Matrix4 &transform,
    bool useFileHeader,
    const std::string &materialType,
    const MaterialParams &materialParams)
    : _material(std::move(material)),
      _transform(transform),
      _invTransform(transform.inverse()),
      _useFileHeader(useFileHeader),
      _materialType(materialType),
      _materialParams(materialParams)
{
    if (!loadSTL(filepath))
        return;
    computeBounds();
    buildBVH();
}

bool STLMesh::isAsciiSTL(std::ifstream &file, std::uintmax_t fileSize)
{
    char buf[6] = {0};
    file.read(buf, 5);
    file.clear();
    file.seekg(0);
    if (std::strncmp(buf, "solid", 5) != 0)
        return false;
    if (fileSize < STL_HEADER_SIZE + 4)
        return true;
    file.seekg(STL_HEADER_SIZE);
    std::uint32_t n = 0;
    file.read(reinterpret_cast<char *>(&n), sizeof(n));
    file.clear();
    file.seekg(0);
    std::uintmax_t expected = STL_HEADER_SIZE + 4
        + static_cast<std::uintmax_t>(n) * STL_TRIANGLE_BYTES;
    return expected != fileSize;
}

void STLMesh::decodeAttributeColor(std::uint16_t attr,
    bool &valid, Color &out)
{
    valid = (attr & 0x8000) != 0;
    if (!valid)
        return;
    double r = ((attr >> 0) & 0x1F) / 31.0;
    double g = ((attr >> 5) & 0x1F) / 31.0;
    double b = ((attr >> 10) & 0x1F) / 31.0;
    out = Color(r, g, b);
}

void STLMesh::decodeMateraliseHeader(const char *header,
    bool &found, Color &out)
{
    found = false;
    std::string h(header, STL_HEADER_SIZE);
    auto pos = h.find("COLOR=");
    if (pos == std::string::npos || pos + 10 > STL_HEADER_SIZE)
        return;
    auto byteAt = [&](std::size_t i) -> int {
        return static_cast<unsigned char>(h[pos + 6 + i]);
    };
    out = Color(byteAt(0) / 255.0, byteAt(1) / 255.0, byteAt(2) / 255.0);
    found = true;
}

void STLMesh::decodeMateraliseMaterial(const char *header,
    bool &found, Color &diffuse, Color &specular)
{
    found = false;
    std::string h(header, STL_HEADER_SIZE);
    auto pos = h.find("MATERIAL=");
    if (pos == std::string::npos || pos + 9 + 12 > STL_HEADER_SIZE)
        return;
    auto byteAt = [&](std::size_t i) -> int {
        return static_cast<unsigned char>(h[pos + 9 + i]);
    };
    diffuse = Color(byteAt(0) / 255.0, byteAt(1) / 255.0, byteAt(2) / 255.0);
    specular = Color(byteAt(4) / 255.0, byteAt(5) / 255.0, byteAt(6) / 255.0);
    found = true;
}

void STLMesh::resolveHeaderMaterial()
{
    if (!_useFileHeader)
        return;
    bool overrode = false;
    if (_stats.hasMaterialiseMaterial) {
        _materialParams.color = _stats.materialiseDiffuse;
        _materialParams.specular = _stats.materialiseSpecular;
        _materialParams.hasSpecular = true;
        overrode = true;
    } else if (_stats.hasMaterialiseColor) {
        _materialParams.color = _stats.materialiseColor;
        overrode = true;
    } else {
        std::cerr << "[STL] use_stl_header requested but no Materialise "
            "header found, falling back to cfg color\n";
        return;
    }
    if (!overrode || _materialType.empty())
        return;
    auto rebuilt = std::shared_ptr<IMaterial>(
        Factory::createMaterial(_materialType, _materialParams));
    if (rebuilt)
        _material = rebuilt;
    else
        std::cerr << "[STL] Failed to rebuild material '"
            << _materialType << "' from header, keeping cfg material\n";
}

bool STLMesh::loadBinary(std::ifstream &file, std::uintmax_t fileSize,
    std::vector<RawTriangle> &raws)
{
    char header[STL_HEADER_SIZE];
    file.read(header, STL_HEADER_SIZE);
    if (file.gcount() != static_cast<std::streamsize>(STL_HEADER_SIZE))
        return false;
    decodeMateraliseHeader(header, _stats.hasMaterialiseColor,
        _stats.materialiseColor);
    decodeMateraliseMaterial(header, _stats.hasMaterialiseMaterial,
        _stats.materialiseDiffuse, _stats.materialiseSpecular);

    std::uint32_t numTriangles = 0;
    file.read(reinterpret_cast<char *>(&numTriangles), sizeof(numTriangles));
    if (file.gcount() != sizeof(numTriangles))
        return false;

    std::uintmax_t expected = STL_HEADER_SIZE + 4
        + static_cast<std::uintmax_t>(numTriangles) * STL_TRIANGLE_BYTES;
    if (expected != fileSize) {
        std::cerr << "[STL] Size mismatch (expected " << expected
            << ", got " << fileSize << "), file may be corrupt\n";
        std::uintmax_t safeMax = (fileSize > STL_HEADER_SIZE + 4)
            ? (fileSize - STL_HEADER_SIZE - 4) / STL_TRIANGLE_BYTES : 0;
        if (numTriangles > safeMax)
            numTriangles = static_cast<std::uint32_t>(safeMax);
    }

    raws.reserve(numTriangles);
    for (std::uint32_t i = 0; i < numTriangles; ++i) {
        STLVec3 rN, r0, r1, r2;
        std::uint16_t attr = 0;
        readBinary(file, rN);
        readBinary(file, r0);
        readBinary(file, r1);
        readBinary(file, r2);
        readBinary(file, attr);
        if (!file)
            return false;
        RawTriangle t{
            Vec3(r0.x, r0.y, r0.z),
            Vec3(r1.x, r1.y, r1.z),
            Vec3(r2.x, r2.y, r2.z),
            Vec3(rN.x, rN.y, rN.z),
            (rN.x != 0.0f || rN.y != 0.0f || rN.z != 0.0f)
        };
        raws.push_back(t);
        bool colorValid = false;
        Color c;
        decodeAttributeColor(attr, colorValid, c);
        if (colorValid)
            ++_stats.coloredTriangles;
    }
    return true;
}

void STLMesh::buildTriangles(const std::vector<RawTriangle> &raws)
{
    _triangles.reserve(raws.size());
    for (const auto &r : raws) {
        auto tri = std::make_unique<STLTriangle>(r.v0, r.v1, r.v2,
            r.fileNormal, _material);
        if (tri->isDegenerate()) {
            ++_stats.degenerate;
            continue;
        }
        _triangles.push_back(std::move(tri));
    }
    _stats.triangles = _triangles.size();
}

bool STLMesh::loadSTL(const std::string &filepath)
{
    std::error_code ec;
    std::uintmax_t fileSize = std::filesystem::file_size(filepath, ec);
    if (ec) {
        std::cerr << "[STL] Cannot stat: " << filepath << '\n';
        return false;
    }
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[STL] Failed to open: " << filepath << '\n';
        return false;
    }

    if (isAsciiSTL(file, fileSize)) {
        std::cerr << "[STL] ASCII STL files are not supported, only binary\n";
        return false;
    }
    if (fileSize < STL_HEADER_SIZE + 4) {
        std::cerr << "[STL] File too small\n";
        return false;
    }
    std::vector<RawTriangle> raws;
    if (!loadBinary(file, fileSize, raws) || raws.empty())
        return false;

    resolveHeaderMaterial();
    buildTriangles(raws);
    return !_triangles.empty();
}

void STLMesh::computeBounds()
{
    if (_triangles.empty()) {
        _bounds = AABB(Vec3(0, 0, 0), Vec3(0, 0, 0));
        return;
    }
    AABB local = _triangles.front()->getBounds();
    for (std::size_t i = 1; i < _triangles.size(); ++i)
        local = local.expand(_triangles[i]->getBounds());
    _bounds = transformAABB(local, _transform);
}

void STLMesh::buildBVH()
{
    std::cout << "Building BVH for STL mesh with " << _triangles.size()
        << " triangles..." << '\n';
    auto start = std::chrono::high_resolution_clock::now();
    _bvh = std::make_unique<BVH>(_triangles);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start);
    std::cout << "STL BVH built in " << duration.count() << "ms" << '\n';
}

bool STLMesh::intersect(const Ray &ray, double tMin, double tMax,
    HitRecord &hit) const
{
    if (!_bvh)
        return false;
    Ray local(_invTransform.transformPoint(ray.origin),
              _invTransform.transformDirection(ray.direction));
    if (!_bvh->intersect(local, tMin, tMax, hit))
        return false;
    hit.point = _transform.transformPoint(hit.point);
    Vec3 worldN = _invTransform.transpose().transformDirection(hit.normal);
    if (worldN.lengthSquared() > EPSILON)
        hit.normal = worldN.normalized();
    return true;
}

bool STLMesh::intersectShadow(const Ray &ray, double tMin, double tMax) const
{
    if (!_bvh)
        return false;
    Ray local(_invTransform.transformPoint(ray.origin),
              _invTransform.transformDirection(ray.direction));
    return _bvh->intersectShadow(local, tMin, tMax);
}

AABB STLMesh::getBounds() const
{
    return _bounds;
}

extern "C"
{
    void rt_plugin_register()
    {
        PrimitiveRegistry::instance().registerType("stl",
            [](const PrimitiveParams &p) -> std::unique_ptr<IPrimitive>
            {
                if (p.objPath.empty())
                    return nullptr;
                return std::make_unique<STLMesh>(p.objPath, p.material,
                    p.transform, p.useFileHeader, p.materialType,
                    p.materialParams);
            }
        );
    }
}
