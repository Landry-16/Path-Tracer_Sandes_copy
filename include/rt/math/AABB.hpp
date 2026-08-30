#ifndef RT_AABB_HPP
#define RT_AABB_HPP

#include "rt/math/Vec3.hpp"
#include "rt/math/Ray.hpp"
#include <algorithm>
#include <limits>

class AABB {
public:
    Vec3 min;
    Vec3 max;

    AABB() : min(Vec3(std::numeric_limits<double>::max(),
                      std::numeric_limits<double>::max(),
                      std::numeric_limits<double>::max())),
             max(Vec3(std::numeric_limits<double>::lowest(),
                      std::numeric_limits<double>::lowest(),
                      std::numeric_limits<double>::lowest())) {}

    AABB(const Vec3 &min, const Vec3 &max) : min(min), max(max) {}

    bool intersect(const Ray &ray, double tMin, double tMax) const {
        for (int i = 0; i < 3; i++) {
            double invD = 1.0 / (i == 0 ? ray.direction.x : i == 1 ? ray.direction.y : ray.direction.z);
            double origin = i == 0 ? ray.origin.x : i == 1 ? ray.origin.y : ray.origin.z;
            double minVal = i == 0 ? min.x : i == 1 ? min.y : min.z;
            double maxVal = i == 0 ? max.x : i == 1 ? max.y : max.z;
            
            double t0 = (minVal - origin) * invD;
            double t1 = (maxVal - origin) * invD;
            
            if (invD < 0.0) {
                std::swap(t0, t1);
            }
            
            tMin = t0 > tMin ? t0 : tMin;
            tMax = t1 < tMax ? t1 : tMax;
            
            if (tMax <= tMin) {
                return false;
            }
        }
        return true;
    }

    AABB expand(const AABB &other) const {
        return AABB(
            Vec3(std::min(min.x, other.min.x),
                 std::min(min.y, other.min.y),
                 std::min(min.z, other.min.z)),
            Vec3(std::max(max.x, other.max.x),
                 std::max(max.y, other.max.y),
                 std::max(max.z, other.max.z))
        );
    }

    AABB expand(const Vec3 &point) const {
        return AABB(
            Vec3(std::min(min.x, point.x),
                 std::min(min.y, point.y),
                 std::min(min.z, point.z)),
            Vec3(std::max(max.x, point.x),
                 std::max(max.y, point.y),
                 std::max(max.z, point.z))
        );
    }

    double surfaceArea() const {
        Vec3 d = max - min;
        return 2.0 * (d.x * d.y + d.y * d.z + d.z * d.x);
    }

    Vec3 centroid() const {
        return (min + max) * 0.5;
    }

    int longestAxis() const {
        Vec3 d = max - min;
        if (d.x > d.y && d.x > d.z) return 0;
        if (d.y > d.z) return 1;
        return 2;
    }

    bool isValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }
};

#endif // RT_AABB_HPP
