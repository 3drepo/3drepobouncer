#pragma optimize("", off)

#include "pmp/algorithms/Manifold.h"
#include "pmp/algorithms/Normals.h"
#include <vector>

using namespace pmp;

Manifold::Manifold(SurfaceMesh& mesh) : mesh(mesh) 
{
}

void Manifold::remove_reflected_edge(Halfedge in, Halfedge out) 
{
    while (mesh.edge(in) == mesh.edge(out))
    {
        auto prev = mesh.prev_halfedge(in);
        auto next = mesh.next_halfedge(out);

        // The edge will be deleted, so update the outgoing edge of its incident
        // vertex if it points to the edge.

        auto v0 = mesh.from_vertex(in);
        if (mesh.halfedge(v0) == in)
        {
            mesh.set_halfedge(v0, next);
        }

        mesh.set_next_halfedge(prev, next);

        mesh.delete_edge(mesh.edge(in));

        // Since the edge turns back in on itself, the vertex at its end will
        // be isolated, so we can delete it with the edge.

        auto v1 = mesh.to_vertex(in);
        mesh.set_halfedge(v1, Halfedge());
        mesh.delete_vertex(v1);

        in = prev;
        out = next;
    }
}

void Manifold::detach_patch(Halfedge start) 
{
    // start is the outgoing half-edge on the right hand side of the fan.
    // First, find the end (incoming, on the left hand side of the fan).

    auto h = start;
    do
    {
        h = mesh.cw_rotated_halfedge(h);
    } while (!mesh.is_boundary(mesh.opposite_halfedge(h)));
    auto end = mesh.opposite_halfedge(h);

    // Update the topology, disconnecting the two patches

    mesh.set_next_halfedge(mesh.prev_halfedge(start), mesh.next_halfedge(end));
    mesh.set_next_halfedge(end, start);

    // Then clone the vertex and redirect the patch to it

    auto v = mesh.clone_vertex(mesh.from_vertex(start));

    h = start;
    do
    {
        mesh.set_vertex(mesh.opposite_halfedge(h), v);
        h = mesh.cw_rotated_halfedge(h);
    } while (mesh.opposite_halfedge(h) != end);

    mesh.set_vertex(end, v);

    mesh.set_halfedge(v, start);
}

void Manifold::fix_manifold()
{
    std::vector<Halfedge> patch;
    for (auto v : mesh.vertices())
    {
        patch.clear();
        auto h = mesh.halfedge(v);

        if(!mesh.is_boundary(v))
        {
            continue; // If the vertex is closed, then there can be no more than one patch
        }

        if (!h.is_valid())
        {
            continue;
        }

        // To begin with, find the end of the primary patch
        auto hend = h;
        do
        {
            h = mesh.cw_rotated_halfedge(h);
            auto o = mesh.opposite_halfedge(h);
            if (mesh.is_boundary(o))
            {
                // If we've completed the loop, then there is only one patch 
                // (the mesh vertex is manifold).
                if (mesh.next_halfedge(o) == hend)
                {
                    break;
                }

                // Otherwise, we have found a second patch, that must be 
                // detached.

                detach_patch(mesh.next_halfedge(o));
            }
        } while (h != hend);
    }
}