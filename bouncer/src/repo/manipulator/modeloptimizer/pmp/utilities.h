// Copyright 2022 the Polygon Mesh Processing Library developers.
// Distributed under a MIT-style license, see LICENSE.txt for details.

#pragma once

#include "pmp/BoundingBox.h"
#include "pmp/SurfaceMesh.h"

namespace pmp {

//! Compute the bounding box of \p mesh .
BoundingBox bounds(const SurfaceMesh& mesh);

//! Flip the orientation of all faces in \p mesh .
void flip_faces(SurfaceMesh& mesh);

//! Checks for possible topological errors in the mesh and throws an exception
//! on encountering such an error.
void check_mesh(SurfaceMesh& mesh);

} // namespace pmp