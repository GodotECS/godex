#include "debug_utilities.h"

#include "core/math/quick_hull.h"
#include "scene/main/scene_tree.h"

struct DrawEdge {
	Vector3 a;
	Vector3 b;
	bool operator<(const DrawEdge &p_edge) const {
		if (a == p_edge.a) {
			return b < p_edge.b;
		} else {
			return a < p_edge.a;
		}
	}

	DrawEdge(const Vector3 &p_a = Vector3(), const Vector3 &p_b = Vector3()) {
		a = p_a;
		b = p_b;
		if (a < b) {
			SWAP(a, b);
		}
	}
};

Ref<ArrayMesh> generate_mesh_from_lines(const Vector<Vector3> &p_lines) {
	Ref<ArrayMesh> mesh = Ref<ArrayMesh>(memnew(ArrayMesh));

	// Create the mesh.
	if (p_lines.is_empty() == false) {
		// make mesh
		Vector<Vector3> array;
		array.resize(p_lines.size());
		{
			Vector3 *w = array.ptrw();
			for (int i = 0; i < p_lines.size(); i++) {
				w[i] = p_lines[i];
			}
		}

		Array arr;
		arr.resize(Mesh::ARRAY_MAX);
		arr[Mesh::ARRAY_VERTEX] = array;

		SceneTree *st = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());

		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, arr);

		if (st) {
			mesh->surface_set_material(0, st->get_debug_collision_material());
		}
	}

	return mesh;
}

Ref<ArrayMesh> generate_mesh_from_points(const Vector<Vector3> &p_points) {
	Vector<Vector3> lines;

	// Extract the lines from the points cloud.
	if (p_points.size() > 0) {
		// Copy the vector.
		Vector<Vector3> varr = Variant(p_points);
		Geometry3D::MeshData md;
		Error err = QuickHull::build(varr, md);
		if (err == OK) {
			lines.resize(md.edges.size() * 2);
			for (uint32_t i = 0; i < md.edges.size(); i++) {
				lines.write[i * 2 + 0] = md.vertices[md.edges[i].vertex_a];
				lines.write[i * 2 + 1] = md.vertices[md.edges[i].vertex_b];
			}
		}
	}

	return generate_mesh_from_lines(lines);
}

Ref<ArrayMesh> generate_mesh_from_faces(const Vector<Vector3> &p_faces) {
	ERR_FAIL_COND_V((p_faces.size() % 3) != 0, nullptr);

	Vector<Vector3> lines;

	// Extract the lines from the faces.
	RBSet<DrawEdge> edges;

	const Vector3 *r = p_faces.ptr();
	for (int i = 0; i < p_faces.size(); i += 3) {
		for (int j = 0; j < 3; j++) {
			DrawEdge de(r[i + j], r[i + ((j + 1) % 3)]);
			edges.insert(de);
		}
	}

	lines.resize(edges.size() * 2);
	int idx = 0;
	for (RBSet<DrawEdge>::Element *E = edges.front(); E; E = E->next()) {
		lines.write[idx + 0] = E->get().a;
		lines.write[idx + 1] = E->get().b;
		idx += 2;
	}

	return generate_mesh_from_lines(lines);
}
