// Copyright 2011-2023 the Polygon Mesh Processing Library developers.
// Distributed under a MIT-style license, see LICENSE.txt for details.

#pragma once

#include "pmp/SurfaceMesh.h"

namespace pmp {

class Diagnostics
{
public:
    static void print_topology(SurfaceMesh& mesh, Vertex v);
    static void print_topology(SurfaceMesh& mesh, Halfedge h);
    static void check_topology(SurfaceMesh& mesh);
    static bool check_manifold(SurfaceMesh& mesh);

};
} // namespace pmp
