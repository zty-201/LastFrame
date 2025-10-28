#pragma once
#include <vector>
#include "BVHTypes.hpp"
#include "BVHUtil.hpp"

// BVH Node
struct BVHNode {
    AABB bounds{};
    int  left = -1;         // child index
    int  right = -1;        // child index
    int  parent = -1;       // parent index
    int  first_prim = -1;   // start index of primitives
    int  prim_count = 0;    // number of primitives
    bool is_leaf() const { return prim_count > 0; }
};

class BVHTree {
public:
    // build a trivial BVH
    void build_flat(const std::vector<BVHPrimitive>& prims);

    // return entities in AABBs
    void query_frustum(const Plane planes[6], std::vector<EntityID>& out) const;
        
    // debug / inspection
    const std::vector<BVHNode>& nodes() const { return m_nodes; }
    const std::vector<BVHPrimitive>& prims() const { return m_prims; }

private:
    std::vector<BVHNode>       m_nodes;
    std::vector<BVHPrimitive>  m_prims;
};
