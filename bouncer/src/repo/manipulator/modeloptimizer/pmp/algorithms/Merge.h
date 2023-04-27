// Copyright 2011-2023 the Polygon Mesh Processing Library developers.
// Distributed under a MIT-style license, see LICENSE.txt for details.

#pragma once

#include "pmp/SurfaceMesh.h"
#include "pmp/BoundingBox.h"
#include <unordered_map>

namespace pmp {

class Merge
{
public:
    Merge(SurfaceMesh& mesh);

    // Merges all vertices that are within the given distance and (normal) angle
    // of eachother, and the edges incident to them as well, where possible.
    void merge(float angle_degress = 30, float tolerance = 0);

private:
    void build_map();
    void merge_vertices();
    void merge_vertices(Vertex v0, Vertex v1);
    void merge_edges();

private:
    SurfaceMesh& mesh;
    std::unordered_map<uint64_t, std::vector<Vertex>> map;
    pmp::BoundingBox bounds;
    float distance_threshold;
    float angle_threshold_radians;
};
} // namespace pmp
