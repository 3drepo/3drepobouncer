#pragma optimize("", off)

#include "pmp/algorithms/Diagnostics.h"

#include <unordered_map>
#include <unordered_set>
#include <set>

// https://dreampuf.github.io/GraphvizOnline

using namespace pmp;

std::set<IndexType> edges;

void add_edge_label(SurfaceMesh& mesh, Halfedge h)
{
    std::cout << h.idx();

    if (mesh.is_boundary(h))
    {
        std::cout << "b";
    }

    std::cout << " (" << mesh.next_halfedge(h).idx();

    if (mesh.is_boundary(mesh.next_halfedge(h)))
    {
        std::cout << "b";
    }

    std::cout << ", " << mesh.prev_halfedge(h).idx();

    if (mesh.is_boundary(mesh.prev_halfedge(h)))
    {
        std::cout << "b";
    }

    std::cout << ")";
}

void draw_halfedge(SurfaceMesh& mesh, Halfedge h)
{
    std::string start = "_" + std::to_string(mesh.prev_halfedge(h).idx()) +
                        "to" + std::to_string(h.idx());
    std::string end = "_" + std::to_string(h.idx()) + "to" +
                      std::to_string(mesh.next_halfedge(h).idx());

    std::cout << start
              << "[label=\"\", shape=\"point\", width=0.1, height=0.1]\n";

    std::cout << end
              << "[label=\"\", shape=\"point\", width=0.1, height=0.1]\n";

    // Create fake vertex to reference

    std::cout << start;

    std::cout << " -> ";

    auto to = mesh.to_vertex(h);
    std::cout << end;

    std::cout << " [label=\"";

    //add_edge_label(mesh, h);

    //std::cout << "\\n";

    //add_edge_label(mesh, mesh.opposite_halfedge(h));

    std::cout << h.idx();

    if (mesh.is_boundary(h))
    {
        std::cout << "b";
    }

    std::cout << "\"]\n";

    std::cout << end << " -> " << to << " [len=0,\"arrowhead\"=none]\n";
}

void print_edge(SurfaceMesh& mesh, Halfedge h)
{
    auto e = mesh.edge(h);
    if (edges.count(e.idx()) == 0)
    {
        edges.insert(e.idx());
    }
    else
    {
        return;
    }

    draw_halfedge(mesh, h);
    draw_halfedge(mesh, mesh.opposite_halfedge(h));
}

void Diagnostics::print_topology(SurfaceMesh& mesh, Halfedge h) 
{
    print_topology(mesh, mesh.from_vertex(h));
    print_topology(mesh, mesh.to_vertex(h));
}

void Diagnostics::print_topology(SurfaceMesh& mesh, Vertex v) 
{
    int n(0);
    auto hit = mesh.halfedges(v);
    auto hend = hit;
    if (hit)
    {
        do
        {
            print_edge(mesh, mesh.prev_halfedge(*hit));
            print_edge(mesh, mesh.next_halfedge(*hit));
            print_edge(mesh, *hit);
        } while (++hit != hend);
    }
}

bool Diagnostics::check_manifold(SurfaceMesh& mesh) 
{
    for (auto v : mesh.vertices())
    {
        if(!mesh.is_manifold(v))
        {
            return false;
        }
    }
    return true;
}