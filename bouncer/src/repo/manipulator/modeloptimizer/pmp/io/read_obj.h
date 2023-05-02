// Copyright 2011-2022 the Polygon Mesh Processing Library developers.
// Distributed under a MIT-style license, see LICENSE.txt for details.

#pragma once

#include "pmp/SurfaceMesh.h"

namespace pmp {

void read_obj(SurfaceMesh& mesh, const std::filesystem::path& file);
void read_obj(SurfaceMesh& mesh, std::basic_istream<char>& in);

} // namespace pmp