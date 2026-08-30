/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** OBJMesh: OBJ parsing, BVH build, and plugin registration
*/

#include "OBJMesh.hpp"
#include <fstream>
#include <sstream>
#include <limits>
#include <iostream>
#include <chrono>
#include "rt/core/Factory.hpp"
#include "rt/core/Registry.hpp"
#include "rt/materials/FlatMaterial.hpp"

using GeomSetter = std::function<void(std::istringstream&,
    std::vector<Vec3>&, std::vector<Vec2>&, std::vector<Vec3>&)>;

static const std::map<std::string, GeomSetter> s_geomParsers = {
    {"v", [](std::istringstream &iss,
             std::vector<Vec3> &vertices,
             std::vector<Vec2>&,
             std::vector<Vec3>&)
        {
            double x, y, z;
            iss >> x >> y >> z;
            vertices.push_back(Vec3(x, y, z));
        }
    },
    {"vt", [](std::istringstream &iss,
              std::vector<Vec3>&,
              std::vector<Vec2> &texcoords,
              std::vector<Vec3>&)
        {
            double u, v;
            iss >> u >> v;
            texcoords.push_back(Vec2(u, v));
        }
    },
    {"vn", [](std::istringstream &iss,
              std::vector<Vec3>&,
              std::vector<Vec2>&,
              std::vector<Vec3> &normals)
        {
            double x, y, z;
            iss >> x >> y >> z;
            normals.push_back(Vec3(x, y, z).normalized());
        }
    },
};

OBJMesh::OBJMesh(const std::string &filepath,
    std::shared_ptr<const IMaterial> material,
    const Matrix4 &transform)
    : transform(transform), invTransform(transform.inverse()),
      defaultMaterial(std::move(material)), fallbackColor(0.82, 0.82, 0.86)
{
    std::filesystem::path objPath(filepath);
    objDirectoryPath = objPath.parent_path();
    objDirectory = objDirectoryPath.string();
    if (!objDirectory.empty() && objDirectory.back() != '/')
        objDirectory += '/';
    if (!loadOBJ(filepath))
        return;
    applyTransformations();
    if (defaultMaterial != nullptr)
        applyMaterialOverride(defaultMaterial);
    else
        assignMissingMaterials();
    computeBounds();
    buildBVH();
}

OBJMesh::~OBJMesh() {}

bool OBJMesh::intersect(const Ray &ray, double tMin, double tMax, HitRecord &hit) const
{
    if (bvh)
        return bvh->intersect(ray, tMin, tMax, hit);
    return false;
}

bool OBJMesh::intersectShadow(const Ray &ray, double tMin, double tMax) const
{
    if (bvh)
        return bvh->intersectShadow(ray, tMin, tMax);
    return false;
}

AABB OBJMesh::getBounds() const
{
    return bounds;
}

void OBJMesh::parseGeometryLine(const std::string &type, std::istringstream &iss,
    std::vector<Vec3> &vertices,
    std::vector<Vec2> &texcoords,
    std::vector<Vec3> &normals)
{
    auto it = s_geomParsers.find(type);
    if (it != s_geomParsers.end())
        it->second(iss, vertices, texcoords, normals);
}

void OBJMesh::parseMaterialLine(const std::string &type, std::istringstream &iss,
    std::string &currentMaterialName,
    std::shared_ptr<const IMaterial> &currentMaterial)
{
    if (type == "mtllib")
    {
        std::string mtlFile;
        iss >> mtlFile;
        loadMTL(objDirectory + mtlFile);
        return;
    }
    if (type == "usemtl")
    {
        iss >> currentMaterialName;
        auto it = materials.find(currentMaterialName);
        currentMaterial = (it != materials.end()) ? it->second.material : nullptr;
    }
}

void OBJMesh::parseFaceIndices(std::istringstream &iss,
    std::vector<int> &vi,
    std::vector<int> &vti,
    std::vector<int> &vni)
{
    std::string vertex;
    while (iss >> vertex)
    {
        std::istringstream vss(vertex);
        std::string sv, st, sn;
        std::getline(vss, sv, '/');
        std::getline(vss, st, '/');
        std::getline(vss, sn, '/');
        if (!sv.empty()) vi.push_back(std::stoi(sv) - 1);
        if (!st.empty()) vti.push_back(std::stoi(st) - 1);
        if (!sn.empty()) vni.push_back(std::stoi(sn) - 1);
    }
}

std::unique_ptr<Triangle> OBJMesh::buildFaceTriangle(
    const std::vector<Vec3> &verts, const std::vector<Vec2> &tcs,
    const std::vector<Vec3> &norms,
    const std::vector<int> &vi, const std::vector<int> &vti,
    const std::vector<int> &vni, size_t i,
    bool hasNorm, bool hasTex,
    std::shared_ptr<const IMaterial> mat) const
{
    if (hasNorm && hasTex)
        return std::make_unique<Triangle>(
            verts[vi[0]], verts[vi[i]], verts[vi[i + 1]],
            norms[vni[0]], norms[vni[i]], norms[vni[i + 1]],
            tcs[vti[0]], tcs[vti[i]], tcs[vti[i + 1]], mat);
    if (hasTex)
        return std::make_unique<Triangle>(
            verts[vi[0]], verts[vi[i]], verts[vi[i + 1]],
            tcs[vti[0]], tcs[vti[i]], tcs[vti[i + 1]], mat);
    return std::make_unique<Triangle>(
        verts[vi[0]], verts[vi[i]], verts[vi[i + 1]], mat);
}

static bool indicesValid(const std::vector<int> &idx, size_t i, int bound)
{
    return idx[0] >= 0 && idx[0] < bound
        && idx[i] >= 0 && idx[i] < bound
        && idx[i + 1] >= 0 && idx[i + 1] < bound;
}

void OBJMesh::addFaceTriangles(
    const std::vector<Vec3> &verts, const std::vector<Vec2> &tcs,
    const std::vector<Vec3> &norms,
    const std::vector<int> &vi, const std::vector<int> &vti,
    const std::vector<int> &vni,
    std::shared_ptr<const IMaterial> mat)
{
    if (vi.size() < 3)
        return;
    bool hasTex = (vti.size() == vi.size() && !tcs.empty());
    bool hasNorm = (vni.size() == vi.size() && !norms.empty());
    for (size_t i = 1; i < vi.size() - 1; i++)
    {
        if (!indicesValid(vi, i, (int)verts.size()))
            continue;
        bool nt = hasNorm && indicesValid(vni, i, (int)norms.size());
        bool tc = hasTex && indicesValid(vti, i, (int)tcs.size());
        triangles.push_back(buildFaceTriangle(verts, tcs, norms, vi, vti, vni, i, nt, tc, mat));
    }
}

void OBJMesh::parseFaceLine(std::istringstream &iss,
    const std::vector<Vec3> &verts,
    const std::vector<Vec2> &tcs,
    const std::vector<Vec3> &norms,
    std::shared_ptr<const IMaterial> mat)
{
    std::vector<int> vi, vti, vni;
    parseFaceIndices(iss, vi, vti, vni);
    addFaceTriangles(verts, tcs, norms, vi, vti, vni, mat);
}

static bool isGeomToken(const std::string &t)
{
    return t == "v" || t == "vt" || t == "vn";
}

static bool isMtlToken(const std::string &t)
{
    return t == "mtllib" || t == "usemtl";
}

bool OBJMesh::loadOBJ(const std::string &filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "[OBJ] Failed to open: " << filepath << std::endl;
        return false;
    }
    std::vector<Vec3> vertices;
    std::vector<Vec2> texcoords;
    std::vector<Vec3> normals;
    std::string line, currentMaterialName;
    std::shared_ptr<const IMaterial> currentMaterial;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        if (isGeomToken(type))
            parseGeometryLine(type, iss, vertices, texcoords, normals);
        else if (isMtlToken(type))
            parseMaterialLine(type, iss, currentMaterialName, currentMaterial);
        else if (type == "f")
            parseFaceLine(iss, vertices, texcoords, normals, currentMaterial);
    }
    file.close();
    return !triangles.empty();
}

static void updateMinMax(Vec3 &minPt, Vec3 &maxPt, const AABB &box)
{
    minPt.x = std::min(minPt.x, box.min.x);
    minPt.y = std::min(minPt.y, box.min.y);
    minPt.z = std::min(minPt.z, box.min.z);
    maxPt.x = std::max(maxPt.x, box.max.x);
    maxPt.y = std::max(maxPt.y, box.max.y);
    maxPt.z = std::max(maxPt.z, box.max.z);
}

static void rebuildTriangleBounds(Triangle *tri)
{
    Vec3 minPoint(
        std::min({tri->v0.x, tri->v1.x, tri->v2.x}),
        std::min({tri->v0.y, tri->v1.y, tri->v2.y}),
        std::min({tri->v0.z, tri->v1.z, tri->v2.z})
    );
    Vec3 maxPoint(
        std::max({tri->v0.x, tri->v1.x, tri->v2.x}),
        std::max({tri->v0.y, tri->v1.y, tri->v2.y}),
        std::max({tri->v0.z, tri->v1.z, tri->v2.z})
    );
    tri->bounds = AABB(minPoint, maxPoint);
}

static void transformTriangle(Triangle *tri, const Matrix4 &tf, const Matrix4 &normalTF)
{
    tri->v0 = tf.transformPoint(tri->v0);
    tri->v1 = tf.transformPoint(tri->v1);
    tri->v2 = tf.transformPoint(tri->v2);
    tri->n0 = normalTF.transformDirection(tri->n0).normalized();
    tri->n1 = normalTF.transformDirection(tri->n1).normalized();
    tri->n2 = normalTF.transformDirection(tri->n2).normalized();
    Vec3 e1 = tri->v1 - tri->v0;
    Vec3 e2 = tri->v2 - tri->v0;
    tri->normal = e1.cross(e2).normalized();
    rebuildTriangleBounds(tri);
}

void OBJMesh::applyTransformations()
{
    Matrix4 normalTransform = invTransform.transpose();
    for (auto &triPtr : triangles)
    {
        Triangle *tri = dynamic_cast<Triangle*>(triPtr.get());
        if (tri)
            transformTriangle(tri, transform, normalTransform);
    }
}

void OBJMesh::applyMaterialOverride(std::shared_ptr<const IMaterial> material)
{
    for (auto &triPtr : triangles)
    {
        Triangle *tri = dynamic_cast<Triangle*>(triPtr.get());
        if (tri)
            tri->material = material;
    }
}

std::shared_ptr<const IMaterial> OBJMesh::getFallbackMaterial()
{
    if (!fallbackMaterial)
    {
        auto mat = std::make_shared<FlatMaterial>(fallbackColor);
        fallbackMaterial = mat;
        ownedMaterials.push_back(mat);
    }
    return fallbackMaterial;
}

void OBJMesh::assignMissingMaterials()
{
    bool needsFallback = false;
    for (const auto &triPtr : triangles)
    {
        const Triangle *tri = dynamic_cast<const Triangle*>(triPtr.get());
        if (tri && !tri->material)
        {
            needsFallback = true;
            break;
        }
    }
    if (!needsFallback)
        return;
    const auto fallback = getFallbackMaterial();
    for (auto &triPtr : triangles)
    {
        Triangle *tri = dynamic_cast<Triangle*>(triPtr.get());
        if (tri && !tri->material)
            tri->material = fallback;
    }
}

void OBJMesh::computeBounds()
{
    if (triangles.empty())
    {
        bounds = AABB(Vec3(0, 0, 0), Vec3(0, 0, 0));
        return;
    }
    const double INF = std::numeric_limits<double>::max();
    Vec3 minPoint(INF, INF, INF);
    Vec3 maxPoint(-INF, -INF, -INF);
    for (const auto &tri : triangles)
        updateMinMax(minPoint, maxPoint, tri->getBounds());
    bounds = AABB(minPoint, maxPoint);
}

void OBJMesh::buildBVH()
{
    std::cout << "Building BVH for mesh with " << triangles.size() << " triangles..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    bvh = std::make_unique<BVH>(triangles);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Mesh BVH built in " << duration.count() << "ms" << std::endl;
}

extern "C"
{
    void rt_plugin_register()
    {
        PrimitiveRegistry::instance().registerType("obj",
            [](const PrimitiveParams &p) -> std::unique_ptr<IPrimitive>
            {
                if (p.objPath.empty())
                    return nullptr;
                return std::make_unique<OBJMesh>(p.objPath, p.material, p.transform);
            }
        );
    }
}
