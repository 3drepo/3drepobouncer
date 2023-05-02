#include "pmp/algorithms/Merge.h"
#include "pmp/algorithms/Normals.h"
#include "pmp/utilities.h"

/**
 * The Compression Algorithm combines vertices co-located in space.
 * Only vertices with normals within the specified range are combined, in order
 * to preserve corners. 
 * This implementation does not consider texture coordinates.
 * 
 * The algorithm works by building a spatial hash of vertex points, and then
 * comparing the distance of vertices within each cell. Vertices below the
 * distance_threshold are combined by redirecting all references to one vertex 
 * to the other, then deleting the original. Afterwards, hidden boundary edges 
 * are removed connecting the incident faces directly.
 * 
 * In this implementation, tolerance is suggestive only: vertices might not be 
 * merged if they sit on the edge of cells. To support his, the cell key should 
 * be made a union so that border cells can be easily queried/the vertex can be 
 * added to bordering cells when the map is constructed.
 */

using namespace pmp;

Merge::Merge(SurfaceMesh& mesh) : mesh(mesh) 
{
    distance_threshold = 0.0;
    angle_threshold_radians = 30.0 * (M_PI / 180.0);

    // The algorithm expects per-vertex normals
    if (!mesh.has_vertex_property("v:normal"))
    {
        Normals::compute_vertex_normals(mesh);
    }
}

void Merge::merge(float angle_degrees, float tolerance) 
{
    this->angle_threshold_radians = angle_degrees * (M_PI / 180.0);
    this->distance_threshold = tolerance;
    build_map();
    merge_vertices();
    merge_edges();
}

void Merge::build_map()
{
    map.clear();
    bounds = BoundingBox();

    // Use a spatial hash to find co-located vertices

    auto positions = mesh.get_vertex_property<pmp::Point>("v:point");
    for (auto& p : positions.vector())
    {
        bounds += p;
    }

    // Each cell key is a 64 bit integer, with 21 bits for each x, y &
    // z component, concatenated together. For simplicity, cells are
    // uniform cubes.

    auto bounds_size = bounds.max() - bounds.min();
    auto max_dimension = std::max(
        std::max(bounds_size(0, 0), bounds_size(1, 0)), bounds_size(2, 0));
    auto min_cell_size = max_dimension / (float)0x1FFFFF;

    auto cell_size = std::max(distance_threshold, min_cell_size);

    for (auto v : mesh.vertices())
    {
        auto position = positions[v] - bounds.min();

        uint64_t xi = position[0] / cell_size;
        uint64_t yi = position[1] / cell_size;
        uint64_t zi = position[2] / cell_size;

        assert((xi & 0x1FFFFF) == xi);
        assert((yi & 0x1FFFFF) == yi);
        assert((zi & 0x1FFFFF) == zi);

        uint64_t cell = (xi << 42) | (yi << 21) | zi;

        map[cell].push_back(v);
    }
}

void Merge::merge_vertices()
{
    std::vector<std::pair<Vertex, Vertex>> merge;

    auto positions = mesh.get_vertex_property<pmp::Point>("v:point");
    auto normals = mesh.get_vertex_property<pmp::Point>("v:normal");

    // Using the map compare potential vertex pairs

    for (auto pair : map)
    {
        auto vertices = pair.second;
        for (size_t i = 0; i < vertices.size(); i++)
        {
            for (size_t j = i + 1; j < vertices.size(); j++)
            {
                auto v0 = vertices[i];
                auto v1 = vertices[j];

                if (distance(positions[v0], positions[v1]) > distance_threshold)
                {
                    continue;
                }

                if(acos(dot(normals[v0], normals[v1])) > angle_threshold_radians)
                {
                    continue;
                }

                merge.push_back({v0, v1});
            }
        }
    }

    for (auto pair : merge)
    {
        if (mesh.is_deleted(pair.first))
        {
            continue;
        }
        if (mesh.is_deleted(pair.second))
        {
            continue;
        }

        merge_vertices(pair.first, pair.second);
    }
}

// Collapses /p v1 into /p v0 and deletes /p v1. Both /p v0 and /p v1 must be
// boundary vertices.
void Merge::merge_vertices(Vertex v0, Vertex v1)
{
    // Check if the vertices are already connected by an edge. If so, that edge
    // needs to be collapsed, which is more involved than simply re-directing 
    // nearby edges.

    auto h = mesh.find_halfedge(v0, v1);
    if (h.is_valid())
    {
        if(mesh.is_collapse_ok(h))
        { 
            mesh.collapse(h); 
        }
        return;
    }

    // Check that this operation is possible; both vertices must have open 
    // corners at which the connection can take place.

    if (!mesh.is_boundary(v0))
    {
        return;
    }
    if (!mesh.is_boundary(v1))
    {
        return;
    }

    mesh.collapse(v0, v1);
}

// Finds mirrored boundary edges that may be part of longer chains left over
// during the vertex merge and combines them.
void Merge::merge_edges()
{
    std::unordered_map<uint64_t, std::vector<Halfedge>> edge_hash;

    for (auto h : mesh.halfedges())
    {
        uint64_t to = mesh.to_vertex(h).idx();
        uint64_t from = mesh.from_vertex(h).idx();

        // Each matching pair will occur twice, once for each direction,
        // so make sure to only capture it once.
        if (to > from)
        {
            continue;
        }

        uint64_t key = (to << 32) | from;
        edge_hash[key].push_back(h);
    }

    for (auto pair : edge_hash)
    {
        auto edges = pair.second;

        if (edges.size() > 1)
        {
            // h1 must be the boundary edge in the pair
            auto h1 = edges[0];
            auto h2 = edges[1];
            if (!mesh.is_boundary(h1))
            {
                std::swap(h1, h2);
            }

            // Make sure we are not touching any edges that have already been deleted.

            if (mesh.is_deleted(h1))
            {
                continue;
            }

            if (mesh.is_deleted(h2))
            {
                continue;
            }

            // We can only combine edges that mirror eachother across a boundary,
            // otherwise we would create complex edges.

            if (mesh.is_boundary(h1) && mesh.is_boundary(mesh.opposite_halfedge(h2)))
            {
                mesh.combine_edges(h1, h2);
            }
        }
    }
}