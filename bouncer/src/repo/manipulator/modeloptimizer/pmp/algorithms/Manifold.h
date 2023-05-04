// Copyright 2011-2023 the Polygon Mesh Processing Library developers.
// Distributed under a MIT-style license, see LICENSE.txt for details.

#pragma once

#include "pmp/SurfaceMesh.h"

namespace pmp {

class Manifold
{
public:
    Manifold(SurfaceMesh& mesh);

    // Adjusts the topology of the mesh to ensure all primitives are manifold
    void fix_manifold();

private:
    SurfaceMesh& mesh;

    void remove_reflected_edge(Halfedge in, Halfedge out);
    void detach_patch(Halfedge rh);
};
} // namespace pmp
