#include "components_mesh_gizmo_3d.h"

#include "../nodes/entity.h"

MeshComponentGizmo::EditorMeshData::EditorMeshData() {
	instance = RenderingServer::get_singleton()->instance_create();
}

MeshComponentGizmo::EditorMeshData::~EditorMeshData() {
	set_mesh(Ref<Mesh>());
	RenderingServer::get_singleton()->free(instance);
}

void MeshComponentGizmo::EditorMeshData::set_mesh(Ref<Mesh> p_mesh) {
	if (p_mesh.is_valid()) {
		base = p_mesh->get_rid();
	} else {
		base = RID();
	}
	RenderingServer::get_singleton()->instance_set_base(instance, base);
}

void MeshComponentGizmo::EditorMeshData::on_position_update(const Transform3D &p_new_transform) {
	RenderingServer::get_singleton()->instance_set_transform(instance, p_new_transform);
}

void MeshComponentGizmo::init() {}

void MeshComponentGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
#ifdef TOOLS_ENABLED
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());

	RID scenario = entity->get_world_3d()->get_scenario();
	Ref<ComponentDepot> component_data = entity->get_component_depot(SNAME("MeshComponent"));
	if (component_data.is_null() || scenario == RID()) {
		// No mesh component, remove any editor mesh.
		Ref<ComponentGizmoData> *editor_mesh = entity->get_internal_entity().gizmo_data.lookup_ptr(SNAME("MeshComponent"));
		if (editor_mesh == nullptr) {
			// Nothing to do.
			return;
		}

		entity->get_internal_entity().gizmo_data.remove(SNAME("MeshComponent"));

	} else {
		// Mesh data, make sure to add the editor mesh.
		Ref<EditorMeshData> editor_mesh;
		{
			Ref<ComponentGizmoData> *d = entity->get_internal_entity().gizmo_data.lookup_ptr(SNAME("MeshComponent"));
			if (d == nullptr || d->is_null()) {
				editor_mesh.instantiate();
				entity->get_internal_entity().gizmo_data.insert(SNAME("MeshComponent"), editor_mesh);
				RenderingServer::get_singleton()->instance_attach_object_instance_id(editor_mesh->instance, entity->get_instance_id());
				RenderingServer::get_singleton()->instance_set_scenario(editor_mesh->instance, scenario);
			} else {
				editor_mesh = *d;
			}
		}

		// Update the mesh.
		Ref<Mesh> m = entity->get_component_value(SNAME("MeshComponent"), SNAME("mesh"));
		editor_mesh->set_mesh(m);

		if (m.is_valid()) {
			Ref<TriangleMesh> tm = m->generate_triangle_mesh();
			if (tm.is_valid()) {
				p_gizmo->add_collision_triangles(tm);
			}
		}

		// Update the transform.

		RenderingServer::get_singleton()->instance_set_transform(editor_mesh->instance, entity->get_global_transform());
		RenderingServer::get_singleton()->instance_set_visible(editor_mesh->instance, entity->is_visible() && component_data->get(SNAME("visible")));
		RenderingServer::get_singleton()->instance_geometry_set_cast_shadows_setting(editor_mesh->instance, (RS::ShadowCastingSetting) int(component_data->get("cast_shadow")));
	}
#endif
}

int MeshComponentGizmo::get_handle_count(const EditorNode3DGizmo *p_gizmo) const {
	return 0;
}

String MeshComponentGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return "";
}

Variant MeshComponentGizmo::get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return Variant();
}

void MeshComponentGizmo::set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
	return;
}

void MeshComponentGizmo::commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
	return;
}
