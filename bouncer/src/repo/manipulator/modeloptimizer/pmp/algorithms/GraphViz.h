// Copyright 2011-2023 the Polygon Mesh Processing Library developers.
// Distributed under a MIT-style license, see LICENSE.txt for details.

#pragma once

#include "pmp/SurfaceMesh.h"

#include <set>

namespace pmp {

/**
* The GraphViz module renders sections of a mesh to Graphviz source. This is
* printed to the console. The output can be rendered by any Graphviz renderer.
* For example, https://dreampuf.github.io/GraphvizOnline can render graphs in
* the browser.
*/
class GraphViz
{
public:
    GraphViz(SurfaceMesh& m);

    void print_topology(Vertex v);
    void print_topology(Halfedge h);

private:
    std::set<IndexType> edges;
    SurfaceMesh& mesh;
    void add_edge_label(Halfedge h);
    void print_edge(Halfedge h);
    void draw_halfedge(Halfedge h);
};
} // namespace pmp
