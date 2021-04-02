#pragma once

#include "scene/resources/mesh.h"

/// Generates a mesh from a group of edges.
Ref<ArrayMesh> generate_mesh_from_lines(const Vector<Vector3> &p_lines);

/// Generates a mesh from a point cloud.
Ref<ArrayMesh> generate_mesh_from_points(const Vector<Vector3> &p_points);

/// Generatges a mesh from a group of faces.
Ref<ArrayMesh> generate_mesh_from_faces(const Vector<Vector3> &p_faces);
