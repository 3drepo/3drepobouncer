// Copyright 2022 the Polygon Mesh Processing Library developers.
// Distributed under a MIT-style license, see LICENSE.txt for details.

#include "pmp/utilities.h"
#include <unordered_map>
#include <set>

namespace pmp {

BoundingBox bounds(const SurfaceMesh& mesh)
{
    BoundingBox bb;
    for (auto v : mesh.vertices())
        bb += mesh.position(v);
    return bb;
}

void flip_faces(SurfaceMesh& mesh)
{
    SurfaceMesh new_mesh;
    for (auto v : mesh.vertices())
    {
        new_mesh.add_vertex(mesh.position(v));
    }
    for (auto f : mesh.faces())
    {
        std::vector<Vertex> vertices;
        for (auto v : mesh.vertices(f))
        {
            vertices.push_back(v);
        }
        std::reverse(vertices.begin(), vertices.end());
        new_mesh.add_face(vertices);
    }
    mesh = new_mesh;
}

void check_mesh(SurfaceMesh& mesh) 
{
    // Check if there are any edges without faces on either side

    for (auto h : mesh.halfedges())
    {
        if(mesh.is_boundary(h) && mesh.is_boundary(mesh.opposite_halfedge(h)))
        {
            auto what = "Found an edge has boundaries on both sides";
            throw pmp::TopologyException(what);
        }
    }

    // Check for phantom references

    std::unordered_map<IndexType, std::set<IndexType>> outgoing_edges;

    // Collect all references reachable from the vertex
    for (auto v : mesh.vertices())
    {
        for (auto h : mesh.halfedges(v))
        {
            outgoing_edges[v.idx()].insert(h.idx());
        }
    }

    // Now check connectivity from the other side (the edges)
    for (auto h : mesh.halfedges())
    {
        auto v = mesh.from_vertex(h).idx();
        auto references = outgoing_edges[v];

        if (references.count(h.idx()) == 0)
        {
            auto what = "Found an edge that references a vertex unreachable from that vertex";
            throw pmp::TopologyException(what);
        }
        assert(references.count(h.idx()) > 0);
    }

    // Check that all edges point to and from only one other half-edge.

    for (auto h : mesh.halfedges())
    {
        if (mesh.next_halfedge(mesh.prev_halfedge(h)) != h)
        {
            auto what = "Found an asymmetrical half-edge association";
            throw pmp::TopologyException(what);
        }

        if(mesh.next_halfedge(mesh.prev_halfedge(h)) != h)
        {
            auto what = "Found an asymmetrical half-edge association";
            throw pmp::TopologyException(what);
        }
        if(mesh.from_vertex(h) == mesh.to_vertex(h))
        {
            auto what = "Found an edge that points to and from the same vertex";
            throw pmp::TopologyException(what);
        }
    }

    // Check that the loops are all complete

    for (auto v : mesh.vertices())
    {
        auto h = mesh.halfedge(v);

        // Make sure h is valid

        if (!mesh.is_valid(h))
        {
            auto what = "Found a vertex that references an invalid halfedge";
            throw pmp::TopologyException(what);
        }

        if (mesh.is_deleted(h))
        {
            auto what = "Found a vertex that references an deleted halfedge";
            throw pmp::TopologyException(what);
        }

        // And that looping through the vertices doesn't get stuck


        auto i = 0;
        auto hend = h;
        do
        {
            h = mesh.cw_rotated_halfedge(h);
            i++;
            if (i > 9999999)
            {
                auto what = "Infinite loop circulating around vertex";
                throw pmp::TopologyException(what);
            }
        } while (h != hend);

        i = 0;
        do
        {
            i++;
            if (i > 9999999)
            {
                auto what = "Infinite loop circulating around vertex";
                throw pmp::TopologyException(what);
            }
            h = mesh.ccw_rotated_halfedge(h);
        } while (h != hend);
    }

    // Make sure that half-edges don't point back into eachother

    for (auto h : mesh.halfedges())
    {
        if (mesh.edge(h) == mesh.edge(mesh.next_halfedge(h)))
        {
            auto what = "Found a halfedge that points back into itself";
            throw pmp::TopologyException(what);

        }

        if (mesh.edge(h) == mesh.edge(mesh.prev_halfedge(h)))
        {
            auto what = "Found a halfedge that points back into itself";
            throw pmp::TopologyException(what);
        }
    }

    // Check that all faces are referenced by their edges

    for (auto f : mesh.faces())
    {
        for (auto h : mesh.halfedges(f))
        {
            if (mesh.face(h) != f)
            {
                auto what = "Found an edge that does not reference its face.";
                throw pmp::TopologyException(what);
            }
        }
    }
}

} // namespace pmp