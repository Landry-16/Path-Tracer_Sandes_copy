/*
** EPITECH PROJECT, 2026
** RAYTRACER
** File description:
** BVH
*/

#ifndef RT_BVH_HPP
    #define RT_BVH_HPP

    #include "rt/math/AABB.hpp"
    #include "rt/interfaces/IPrimitive.hpp"
    #include "rt/math/Ray.hpp"
    #include <vector>
    #include <memory>
    #include <algorithm>

struct BVHNode {
    AABB bounds;
    std::unique_ptr<BVHNode> left;
    std::unique_ptr<BVHNode> right;
    std::vector<const IPrimitive*> primitives;
    
    bool isLeaf() const {
        return left == nullptr && right == nullptr;
    }
};

class BVH {
public:
    BVH(const std::vector<std::unique_ptr<IPrimitive>> &prims);
    
    bool intersect(const Ray &ray, double tMin, double tMax, HitRecord &hit) const;
    bool intersectShadow(const Ray &ray, double tMin, double tMax) const;

private:
    std::unique_ptr<BVHNode> root;
    
    std::unique_ptr<BVHNode> build(std::vector<const IPrimitive*> &primitives, int start, int end);
    bool intersectNode(const BVHNode *node, const Ray &ray, double tMin, double tMax, HitRecord &hit) const;
    bool intersectShadowNode(const BVHNode *node, const Ray &ray, double tMin, double tMax) const;
    
    AABB computeBounds(const std::vector<const IPrimitive*> &primitives, int start, int end) const;
    int findBestSplit(const std::vector<const IPrimitive*> &primitives, int start, int end, int axis) const;

    struct Bucket { int count = 0; AABB bounds; };
    std::pair<double, double> computeAxisRange(const std::vector<const IPrimitive*> &primitives,
                                               int start, int end, int axis) const;
    void fillBuckets(const std::vector<const IPrimitive*> &primitives, int start, int end,
                     int axis, double axisMin, double axisMax, Bucket buckets[]) const;
    void computeBucketCosts(const Bucket buckets[], double cost[], int n) const;
    int findMinCostBucketIndex(const double cost[], int n) const;
    int partitionAtBucket(std::vector<const IPrimitive*> &primitives, int start, int end,
                          int axis, double axisMin, double axisMax, int splitIdx) const;
};

#endif // RT_BVH_HPP
