// Copyright 2011-2022 the Polygon Mesh Processing Library developers.
// Distributed under a MIT-style license, see LICENSE.txt for details.

#include "pmp/io/read_pmp.h"

#include "pmp/io/helpers.h"

namespace pmp {

void read_pmp(SurfaceMesh& mesh, const std::filesystem::path& file)
{
    // open file (in binary mode)
    FILE* in = fopen(file.string().c_str(), "rb");
    if (!in)
        throw IOException("Failed to open file: " + file.string());

    // how many elements?
    size_t nv{0};
    size_t ne{0};
    size_t nf{0};
    tfread(in, nv);
    tfread(in, ne);
    tfread(in, nf);
    auto nh = 2 * ne;

    // texture coordinates?
    bool has_htex{false};
    tfread(in, has_htex);

    bool has_vnormal{false};
    tfread(in, has_vnormal);    

    // resize containers
    mesh.vprops_.resize(nv);
    mesh.hprops_.resize(nh);
    mesh.eprops_.resize(ne);
    mesh.fprops_.resize(nf);

    // read properties from file
    // clang-format off
    size_t nvc = fread((char*)mesh.vconn_.data(), sizeof(SurfaceMesh::VertexConnectivity), nv, in);
    size_t nhc = fread((char*)mesh.hconn_.data(), sizeof(SurfaceMesh::HalfedgeConnectivity), nh, in);
    size_t nfc = fread((char*)mesh.fconn_.data(), sizeof(SurfaceMesh::FaceConnectivity), nf, in);
    size_t np = fread((char*)mesh.vpoint_.data(), sizeof(Point), nv, in);
    // clang-format on

    PMP_ASSERT(nvc == nv);
    PMP_ASSERT(nhc == nh);
    PMP_ASSERT(nfc == nf);
    PMP_ASSERT(np == nv);

    // read texture coordiantes
    if (has_htex)
    {
        auto htex = mesh.halfedge_property<TexCoord>("h:tex");
        size_t nhtc = fread((char*)htex.data(), sizeof(TexCoord), nh, in);
        PMP_ASSERT(nhtc == nh);
    }

    if (has_vnormal)
    {
        auto vnormal = mesh.vertex_property<Normal>("v:normal");
        size_t nvn = fread((char*)vnormal.data(), sizeof(Normal), nv, in);
        PMP_ASSERT(nvn == nv);
    }

    fclose(in);
}

} // namespace pmp
