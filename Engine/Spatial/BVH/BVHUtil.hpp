#pragma once
#include "BVHTypes.hpp"

// Plane in n.x + d = 0
// fill an array of 6 planes from camera
struct Plane { float nx, ny, nz, d; };

// Fast plane vs AABB (returns true if box is fully outside any plane)
inline bool aabb_outside_plane(const AABB& b, const Plane& p) {
    // pick AABB vertex most likely to be outside (positive towards plane normal)
    float x = p.nx >= 0 ? b.max[0] : b.min[0];
    float y = p.ny >= 0 ? b.max[1] : b.min[1];
    float z = p.nz >= 0 ? b.max[2] : b.min[2];
    return (p.nx * x + p.ny * y + p.nz * z + p.d) < 0.0f;
}

// returns true if box is not outside any plane
inline bool aabb_in_frustum(const AABB& b, const Plane planes[6]) {
    for (int i = 0; i < 6; ++i) {
        if (aabb_outside_plane(b, planes[i])) return false;
    }
    return true;
}
