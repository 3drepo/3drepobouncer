#include "pmp/algorithms/GraphViz.h"

#include <unordered_map>
#include <unordered_set>

// https://dreampuf.github.io/GraphvizOnline

using namespace pmp;

GraphViz::GraphViz(SurfaceMesh& m):mesh(m) {
}

void GraphViz::add_edge_label(Halfedge h)
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

void GraphViz::draw_halfedge(Halfedge h)
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

    std::cout << h.idx();

    if (mesh.is_boundary(h))
    {
        std::cout << "b";
    }

    std::cout << "\"]\n";

    std::cout << end << " -> " << to << " [len=0,\"arrowhead\"=none]\n";
}

void GraphViz::print_edge(Halfedge h)
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

    draw_halfedge(h);
    draw_halfedge(mesh.opposite_halfedge(h));
}

void GraphViz::print_topology(Halfedge h) 
{
    print_topology(mesh.from_vertex(h));
    print_topology(mesh.to_vertex(h));
}

void GraphViz::print_topology(Vertex v) 
{
    int n(0);
    auto hit = mesh.halfedges(v);
    auto hend = hit;
    if (hit)
    {
        do
        {
            print_edge(mesh.prev_halfedge(*hit));
            print_edge(mesh.next_halfedge(*hit));
            print_edge(*hit);
        } while (++hit != hend);
    }
}