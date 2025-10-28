#pragma once
#include <array>
#include <cstdint>
#include <limits>

using EntityID = uint32_t;

// lower and upper corners
struct AABB {
    std::array<float, 3> min{
        +std::numeric_limits<float>::infinity(),
        +std::numeric_limits<float>::infinity(),
        +std::numeric_limits<float>::infinity()
    };
    std::array<float, 3> max{
        -std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity()
    };
};

// grows so it will enclose other
inline void expand(AABB& b, const AABB& other) {
    for (int i = 0; i < 3; ++i) {
        b.min[i] = std::min(b.min[i], other.min[i]);
        b.max[i] = std::max(b.max[i], other.max[i]);
    }
}

// records entity numbers and dependency
struct BVHPrimitive {
    AABB     bounds;
    EntityID entity = 0;
    uint32_t id = 0;
};
