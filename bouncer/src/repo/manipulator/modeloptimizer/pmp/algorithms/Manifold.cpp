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

void Manifold::fix_manifold()
{
    auto points = mesh.get_vertex_property<Point>("v:point");
    std::vector<Halfedge> second_corner;
    for (auto v : mesh.vertices())
    {
        second_corner.clear();
        auto h = mesh.halfedge(v);
        auto hend = h;

        // This is the boundary edge of the primary patch (the one that won't
        // be detached). Moving from corner_start (which we find below) to
        // corner_end should be across a set of faces with no gaps.
        Halfedge corner_end;

        // Since we are changing the topology in this method, step through
        // the edges manually rather than using the iterator.

        if (h.is_valid())
        {
            do
            {
                auto nh = mesh.ccw_rotated_halfedge(h); // If not changed below, nh will be the next outgoing halfedge around v

                if (corner_end.is_valid())
                {
                    // If we've already encountered a boundary, start tracking
                    // additional edges, because they may form their own corner.

                    second_corner.push_back(h);
                }

                if (mesh.is_boundary(h))
                {
                    if (!corner_end.is_valid())
                    {
                        corner_end = h;
                    }
                    else
                    {
                        // We have encountered a second outgoing boundary, and
                        // so patch - we need to detach the corner into its own 
                        // patch.

                        // corner_start is the edge on the opposite side of the
                        // primary patch to corner_end.

                        auto corner_start =
                            mesh.prev_halfedge(second_corner[second_corner.size() - 1]);

                        // First split the two faces at v by turning the figure-
                        // of-eight edge-loop into two closed loops.

                        // (a & b are the incoming/outgoing edges that delineate
                        // the patch to be detached).

                        auto a = mesh.prev_halfedge(corner_end);
                        auto b = mesh.next_halfedge(corner_start);

                        // Make the connections..

                        mesh.set_next_halfedge(corner_start, corner_end);
                        mesh.set_next_halfedge(a, b);

                        // Make a copy of the vertex to re-direct the new patch to
                        auto p = points[v];
                        auto copy = mesh.add_vertex(p);

                        for (auto ch : second_corner)
                        {
                            mesh.set_vertex(mesh.opposite_halfedge(ch), copy);
                        }

                        mesh.set_halfedge(copy, second_corner[0]);

                        // We are done with the corner patch. There may be more
                        // patches to detach, so keep the corner_start edge
                        // around.

                        second_corner.clear();

                        // Update the iterator so we keep to the main patch

                        nh = mesh.opposite_halfedge(corner_start);

                        // Check if we have introduced a reflected edge, and if
                        // so delete it. Do this after the new topology has been 
                        // completely finished, as it will delete the attached
                        // vertices.

                        if (mesh.edge(corner_start) == mesh.edge(corner_end))
                        {
                            remove_reflected_edge(corner_start, corner_end);
                        }

                        if (mesh.edge(a) == mesh.edge(b))
                        {
                            remove_reflected_edge(a, b);
                        }
                    }
                }

                h = nh;
            } while (h != hend);
        }
    }
}