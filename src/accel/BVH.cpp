/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** BVH
*/

#include "rt/rendering/BVH.hpp"
#include <limits>
#include <iostream>

constexpr int MAX_PRIMS_PER_LEAF = 4;
constexpr int NUM_BUCKETS = 12;

BVH::BVH(const std::vector<std::unique_ptr<IPrimitive>> &prims)
{
    if (prims.empty()) {
        root = nullptr;
        return;
    }
    
    std::vector<const IPrimitive*> primPtrs;
    primPtrs.reserve(prims.size());
    for (const auto &prim : prims) {
        AABB bounds = prim->getBounds();
        if (std::isfinite(bounds.min.x) && std::isfinite(bounds.max.x)) {
            primPtrs.push_back(prim.get());
        }
    }
    
    if (primPtrs.empty()) {
        root = nullptr;
        return;
    }
    
    root = build(primPtrs, 0, primPtrs.size());
}

AABB BVH::computeBounds(const std::vector<const IPrimitive*> &primitives, int start, int end) const
{
    AABB bounds;
    for (int i = start; i < end; i++) {
        AABB primBounds = primitives[i]->getBounds();
        if (primBounds.isValid()) {
            bounds = bounds.expand(primBounds);
        }
    }
    return bounds;
}

std::pair<double, double> BVH::computeAxisRange(const std::vector<const IPrimitive*> &primitives,
                                                 int start, int end, int axis) const
{
    AABB centroidBounds;
    for (int i = start; i < end; i++) {
        centroidBounds = centroidBounds.expand(primitives[i]->getBounds().centroid());
    }
    double axisMin = axis == 0 ? centroidBounds.min.x : axis == 1 ? centroidBounds.min.y : centroidBounds.min.z;
    double axisMax = axis == 0 ? centroidBounds.max.x : axis == 1 ? centroidBounds.max.y : centroidBounds.max.z;
    return {axisMin, axisMax};
}

void BVH::fillBuckets(const std::vector<const IPrimitive*> &primitives, int start, int end,
                      int axis, double axisMin, double axisMax, Bucket buckets[]) const
{
    for (int i = start; i < end; i++) {
        Vec3 centroid = primitives[i]->getBounds().centroid();
        double val = axis == 0 ? centroid.x : axis == 1 ? centroid.y : centroid.z;
        int idx = NUM_BUCKETS * ((val - axisMin) / (axisMax - axisMin));
        if (idx == NUM_BUCKETS) idx = NUM_BUCKETS - 1;
        buckets[idx].count++;
        buckets[idx].bounds = buckets[idx].bounds.expand(primitives[i]->getBounds());
    }
}

void BVH::computeBucketCosts(const Bucket buckets[], double cost[], int n) const
{
    for (int i = 0; i < n; i++) {
        AABB b0, b1;
        int count0 = 0, count1 = 0;
        for (int j = 0; j <= i; j++) { b0 = b0.expand(buckets[j].bounds); count0 += buckets[j].count; }
        for (int j = i + 1; j < NUM_BUCKETS; j++) { b1 = b1.expand(buckets[j].bounds); count1 += buckets[j].count; }
        cost[i] = 0.125 + (count0 * b0.surfaceArea() + count1 * b1.surfaceArea());
    }
}

int BVH::findMinCostBucketIndex(const double cost[], int n) const
{
    int minIdx = 0;
    for (int i = 1; i < n; i++) {
        if (cost[i] < cost[minIdx]) minIdx = i;
    }
    return minIdx;
}

int BVH::partitionAtBucket(std::vector<const IPrimitive*> &primitives, int start, int end,
                            int axis, double axisMin, double axisMax, int splitIdx) const
{
    auto midIter = std::partition(
        const_cast<const IPrimitive**>(&primitives[start]),
        const_cast<const IPrimitive**>(&primitives[end]),
        [&](const IPrimitive *prim) {
            Vec3 centroid = prim->getBounds().centroid();
            double val = axis == 0 ? centroid.x : axis == 1 ? centroid.y : centroid.z;
            int idx = NUM_BUCKETS * ((val - axisMin) / (axisMax - axisMin));
            if (idx == NUM_BUCKETS) idx = NUM_BUCKETS - 1;
            return idx <= splitIdx;
        }
    );
    return midIter - &primitives[0];
}

int BVH::findBestSplit(const std::vector<const IPrimitive*> &primitives, int start, int end, int axis) const
{
    int count = end - start;
    if (count <= MAX_PRIMS_PER_LEAF)
        return -1;
    auto [axisMin, axisMax] = computeAxisRange(primitives, start, end, axis);
    if (axisMax - axisMin < 1e-8)
        return -1;
    Bucket buckets[NUM_BUCKETS];
    fillBuckets(primitives, start, end, axis, axisMin, axisMax, buckets);
    double cost[NUM_BUCKETS - 1];
    computeBucketCosts(buckets, cost, NUM_BUCKETS - 1);
    int minCostIdx = findMinCostBucketIndex(cost, NUM_BUCKETS - 1);
    AABB leafBounds = computeBounds(primitives, start, end);
    if (cost[minCostIdx] >= count * leafBounds.surfaceArea() || count <= MAX_PRIMS_PER_LEAF)
        return -1;
    return partitionAtBucket(const_cast<std::vector<const IPrimitive*>&>(primitives),
                             start, end, axis, axisMin, axisMax, minCostIdx);
}


std::unique_ptr<BVHNode> BVH::build(std::vector<const IPrimitive*> &primitives, int start, int end)
{
    auto node = std::make_unique<BVHNode>();
    node->bounds = computeBounds(primitives, start, end);
    
    int count = end - start;
    
    if (count <= MAX_PRIMS_PER_LEAF) {
        node->primitives.assign(primitives.begin() + start, primitives.begin() + end);
        return node;
    }
    
    int axis = node->bounds.longestAxis();
    int mid = findBestSplit(primitives, start, end, axis);
    
    if (mid == -1 || mid == start || mid == end) {
        node->primitives.assign(primitives.begin() + start, primitives.begin() + end);
        return node;
    }
    
    node->left = build(primitives, start, mid);
    node->right = build(primitives, mid, end);
    
    return node;
}

bool BVH::intersect(const Ray &ray, double tMin, double tMax, HitRecord &hit) const
{
    if (!root)
        return false;
    return intersectNode(root.get(), ray, tMin, tMax, hit);
}

bool BVH::intersectNode(const BVHNode *node, const Ray &ray, double tMin, double tMax, HitRecord &hit) const
{
    if (!node->bounds.intersect(ray, tMin, tMax)) {
        return false;
    }
    if (node->isLeaf()) {
        bool hitAnything = false;
        double closest = tMax;
        
        for (const auto *prim : node->primitives) {
            HitRecord tempHit;
            if (prim->intersect(ray, tMin, closest, tempHit)) {
                hitAnything = true;
                closest = tempHit.t;
                hit = tempHit;
            }
        }   
        return hitAnything;
    }
    bool hitLeft = intersectNode(node->left.get(), ray, tMin, tMax, hit);
    double rightTMax = hitLeft ? hit.t : tMax;
    bool hitRight = intersectNode(node->right.get(), ray, tMin, rightTMax, hit);
    return hitLeft || hitRight;
}

bool BVH::intersectShadow(const Ray &ray, double tMin, double tMax) const
{
    if (!root)
        return false;
    return intersectShadowNode(root.get(), ray, tMin, tMax);
}

bool BVH::intersectShadowNode(const BVHNode *node, const Ray &ray, double tMin, double tMax) const
{
    if (!node->bounds.intersect(ray, tMin, tMax)) {
        return false;
    }
    
    if (node->isLeaf()) {
        for (const auto *prim : node->primitives) {
            if (prim->intersectShadow(ray, tMin, tMax)) {
                return true;
            }
        }
        return false;
    }
    
    if (intersectShadowNode(node->left.get(), ray, tMin, tMax)) {
        return true;
    }
    
    return intersectShadowNode(node->right.get(), ray, tMin, tMax);
}
