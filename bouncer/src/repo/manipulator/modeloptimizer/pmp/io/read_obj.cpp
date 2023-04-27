// Copyright 2011-2022 the Polygon Mesh Processing Library developers.
// Distributed under a MIT-style license, see LICENSE.txt for details.

#include "pmp/io/read_obj.h"
#include <unordered_map>

namespace pmp {

void read_obj(SurfaceMesh& mesh, const std::filesystem::path& file)
{
    std::array<char, 200> s;
    float x, y, z;
    std::vector<Vertex> face_vertices;
    std::vector<std::tuple<int, int, int>> face;
    std::tuple<int, int, int> attributes = {0, 0, 0};

    bool with_tex_coord = false;
    bool with_normals = false;
    auto vertex_tex = mesh.add_vertex_property<TexCoord>("v:tex");
    auto vertex_normals = mesh.add_vertex_property<Normal>("v:normal");

    struct attribute_hasher
    {
        std::size_t operator()(std::tuple<int, int, int> const& t) const noexcept
        {
            uint64_t a0 = std::get<0>(t);
            uint64_t a1 = std::get<1>(t);
            uint64_t a2 = std::get<2>(t);
            return (a0 << 42) | (a1 << 21) | (a0);
        }
    };

    // Maps between attribute combinations and the face that match them
    std::unordered_map<std::tuple<int, int, int>, Vertex, attribute_hasher> vertex_map;

    // Lists of attributes as ordered in the OBJ
    std::vector<Point> all_positions;
    std::vector<Normal> all_normals;
    std::vector<TexCoord> all_tex_coords; 
   
    // open file (in ASCII mode)
    FILE* in = fopen(file.string().c_str(), "r");
    if (!in)
        throw IOException("Failed to open file: " + file.string());

    // clear line once
    memset(s.data(), 0, 200);

    // parse line by line (currently only supports vertex all_positions & faces
    while (in && !feof(in) && fgets(s.data(), 200, in))
    {
        // comment
        if (s[0] == '#' || isspace(s[0]))
            continue;

        // vertex
        else if (strncmp(s.data(), "v ", 2) == 0)
        {
            if (sscanf(s.data(), "v %f %f %f", &x, &y, &z))
            {
                all_positions.push_back(Point(x, y, z));
            }
        }

        // normal
        else if (strncmp(s.data(), "vn ", 3) == 0)
        {
            if (sscanf(s.data(), "vn %f %f %f", &x, &y, &z))
            {
                all_normals.push_back(Normal(x, y, z));
            }
        }

        // texture coordinate
        else if (strncmp(s.data(), "vt ", 3) == 0)
        {
            if (sscanf(s.data(), "vt %f %f", &x, &y))
            {
                all_tex_coords.push_back(TexCoord(x, y));
            }
        }

        // face
        else if (strncmp(s.data(), "f ", 2) == 0)
        {
            int component(0);
            bool end_of_vertex(false);
            char *p0, *p1(s.data() + 1);

            face.clear();

            // skip white-spaces
            while (*p1 == ' ')
                ++p1;

            while (p1)
            {
                p0 = p1;

                // overwrite next separator

                // skip '/', '\n', ' ', '\0', '\r' <-- don't forget Windows
                while (*p1 != '/' && *p1 != '\r' && *p1 != '\n' && *p1 != ' ' &&
                       *p1 != '\0')
                    ++p1;

                // detect end of vertex
                if (*p1 != '/')
                {
                    end_of_vertex = true;
                }

                // replace separator by '\0'
                if (*p1 != '\0')
                {
                    *p1 = '\0';
                    p1++; // point to next token
                }

                // detect end of line and break
                if (*p1 == '\0' || *p1 == '\n')
                {
                    p1 = nullptr;
                }

                // read next vertex component
                if (*p0 != '\0')
                {
                    switch (component)
                    {
                        case 0: // vertex
                        {
                            int idx = atoi(p0);
                            if (idx < 0)
                                idx = all_positions.size() + idx + 1;
                            std::get<0>(attributes) = idx - 1;

                            break;
                        }
                        case 1: // texture coord
                        {
                            int idx = atoi(p0);
                            if (idx < 0)
                                idx = all_tex_coords.size() + idx + 1;
                            std::get<1>(attributes) = idx - 1;
                            with_tex_coord = true;
                            break;
                        }
                        case 2: // normal
                        {
                            int idx = atoi(p0);
                            if (idx < 0)
                                idx = all_normals.size() + idx + 1;
                            std::get<2>(attributes) = idx - 1;
                            with_normals = true;
                            break;
                        }
                    }
                }

                ++component;

                if (end_of_vertex)
                {
                    component = 0;
                    end_of_vertex = false;
                    face.push_back(attributes);
                    attributes = {0, 0, 0};
                }
            }

            face_vertices.clear();
            for (auto v : face)
            {
                auto existing_vertex = vertex_map.find(v);
                if (existing_vertex == vertex_map.end())
                {
                    auto m_vertex = mesh.add_vertex(
                        all_positions[std::get<0>(v)]);
                    if (with_tex_coord)
                    {
                        vertex_tex[m_vertex] =
                            all_tex_coords[std::get<1>(v)];
                    }
                    if (with_normals)
                    {
                        vertex_normals[m_vertex] =
                            all_normals[std::get<2>(v)];
                    }
                    vertex_map[v] = m_vertex;
                    face_vertices.push_back(m_vertex);
                }
                else
                {
                    face_vertices.push_back(existing_vertex->second);
                }
            }

            Face f = mesh.add_face(face_vertices);
        }
        // clear line
        memset(s.data(), 0, 200);
    }

    // if there are no textures, delete texture property!
    if (!with_tex_coord)
    {
        mesh.remove_vertex_property(vertex_tex);
    }
    if (!with_normals)
    {
        mesh.remove_vertex_property(vertex_normals);
    }

    fclose(in);
}

} // namespace pmp