/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** Texture path resolution helpers for OBJMesh
*/

#include "OBJMesh.hpp"
#include <algorithm>
#include <system_error>

std::string OBJMesh::trim(const std::string &value)
{
    const auto start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

std::vector<std::filesystem::path> OBJMesh::buildSearchRoots() const
{
    namespace fs = std::filesystem;
    std::vector<fs::path> roots;
    if (objDirectoryPath.empty())
        return roots;
    roots.push_back(objDirectoryPath);
    auto parent = objDirectoryPath.parent_path();
    if (!parent.empty())
    {
        roots.push_back(parent);
        auto grand = parent.parent_path();
        if (!grand.empty())
            roots.push_back(grand);
    }
    return roots;
}

std::string OBJMesh::searchRelativeInRoots(const std::filesystem::path &rel,
    const std::vector<std::filesystem::path> &roots) const
{
    namespace fs = std::filesystem;
    if (rel.empty())
        return {};
    for (const auto &root : roots)
    {
        std::error_code ec;
        auto c1 = root / rel;
        if (fs::exists(c1, ec) && fs::is_regular_file(c1, ec))
            return c1.lexically_normal().string();
        auto c2 = root / "textures" / rel;
        if (fs::exists(c2, ec) && fs::is_regular_file(c2, ec))
            return c2.lexically_normal().string();
    }
    return {};
}

std::string OBJMesh::tryExtensionVariants(const std::filesystem::path &filename,
    const std::vector<std::filesystem::path> &roots) const
{
    static const std::vector<std::string> exts = {
        ".png", ".jpg", ".jpeg", ".PNG", ".JPG", ".JPEG"
    };
    std::string stem = filename.stem().string();
    if (stem.empty())
        return {};
    for (const auto &ext : exts)
    {
        std::filesystem::path alt = stem + ext;
        if (alt == filename)
            continue;
        if (auto found = searchRelativeInRoots(alt, roots); !found.empty())
            return found;
    }
    return {};
}

std::string OBJMesh::resolveTexturePath(const std::string &texPath, bool mustExist) const
{
    namespace fs = std::filesystem;
    std::string cleaned = trim(texPath);
    if (cleaned.empty())
        return "";
    auto tryFile = [](const fs::path &p) -> std::string
    {
        std::error_code ec;
        if (!p.empty() && fs::exists(p, ec) && fs::is_regular_file(p, ec))
            return p.lexically_normal().string();
        return {};
    };
    fs::path raw(cleaned);
    if (raw.is_absolute())
    {
        if (auto f = tryFile(raw); !f.empty())
            return f;
        raw = raw.filename();
    }
    auto roots = buildSearchRoots();
    if (auto f = searchRelativeInRoots(raw, roots); !f.empty())
        return f;
    if (auto f = searchRelativeInRoots(raw.filename(), roots); !f.empty())
        return f;
    if (auto f = tryExtensionVariants(raw.filename(), roots); !f.empty())
        return f;
    if (!mustExist)
        return (!objDirectoryPath.empty()
            ? (objDirectoryPath / raw).lexically_normal().string()
            : raw.lexically_normal().string());
    return "";
}

std::vector<std::string> OBJMesh::buildMaterialNameVariants(const std::string &name) const
{
    std::vector<std::string> variants{name};
    auto addUnique = [&](std::string s)
    {
        if (!s.empty() && std::find(variants.begin(), variants.end(), s) == variants.end())
            variants.push_back(std::move(s));
    };
    std::string underscored = name;
    std::replace(underscored.begin(), underscored.end(), ' ', '_');
    addUnique(underscored);
    std::string dashed = underscored;
    std::replace(dashed.begin(), dashed.end(), '_', '-');
    addUnique(dashed);
    std::string dotted = name;
    std::replace(dotted.begin(), dotted.end(), ' ', '.');
    addUnique(dotted);
    return variants;
}

std::string OBJMesh::findFallbackDiffuseTexture(const std::string &materialName) const
{
    if (materialName.empty())
        return "";
    static const std::vector<std::string> suffixes = {
        "_Base_Color", "_base_color", "_BaseColor", "_baseColor",
        "_Albedo", "_albedo", "_Color", "_color", "_Diffuse", "_diffuse"
    };
    static const std::vector<std::string> exts = {
        ".png", ".jpg", ".jpeg", ".PNG", ".JPG", ".JPEG"
    };
    auto variants = buildMaterialNameVariants(materialName);
    for (const auto &base : variants)
        for (const auto &suf : suffixes)
            for (const auto &ext : exts)
                if (auto r = resolveTexturePath(base + suf + ext, true); !r.empty())
                    return r;
    return "";
}
