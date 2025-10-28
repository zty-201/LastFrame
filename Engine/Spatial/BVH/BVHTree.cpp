#include "BVHTree.hpp"
#include <algorithm>

// build flat bvh
void BVHTree::build_flat(const std::vector<BVHPrimitive>& prims) {
    m_prims = prims;
    m_nodes.clear();
    m_nodes.resize(1); // reset nodes and allocate one node

    BVHNode& root = m_nodes[0];
    root.first_prim = 0;
    root.prim_count = static_cast<int>(m_prims.size());

    // compute root bounds
    for (const auto& p : m_prims) {
        expand(root.bounds, p.bounds);
    }
}

// frustum culling
void BVHTree::query_frustum(const Plane planes[6], std::vector<EntityID>& out) const {
    out.clear();
    if (m_nodes.empty()) return;

    for (const auto& p : m_prims) {
        if (aabb_in_frustum(p.bounds, planes)) {
            out.push_back(p.entity);    // entity ID
        }
    }
}
