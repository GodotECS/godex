#include "components_gizmo_3d.h"

#include "../nodes/entity.h"
#include "servers/rendering_server.h"

Components3DGizmoPlugin::Components3DGizmoPlugin() {
}

void Components3DGizmoPlugin::add_component_gizmo(Ref<ComponentGizmo> p_gizmo) {
	ERR_FAIL_COND(p_gizmo.is_null());
	ERR_FAIL_COND(p_gizmo->owner != nullptr);
	gizmos.push_back(p_gizmo);
}

bool Components3DGizmoPlugin::has_gizmo(Node3D *p_node) {
	return Object::cast_to<Entity3D>(p_node) != nullptr;
}

String Components3DGizmoPlugin::get_gizmo_name() const {
	return "Components3DGizmoPlugin";
}

int Components3DGizmoPlugin::get_priority() const {
	return -1;
}

void Components3DGizmoPlugin::redraw(EditorNode3DGizmo *p_gizmo) {
	p_gizmo->clear();
	for (uint32_t i = 0; i < gizmos.size(); i += 1) {
		if (gizmos[i]->is_init == false) {
			gizmos[i]->owner = this;
			gizmos[i]->init();
			gizmos[i]->is_init = true;
		}
		gizmos[i]->redraw(p_gizmo);
	}
}

String Components3DGizmoPlugin::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	// Find the gizmo ID.
	RelativeHandle relative_handle = find_gizmo_by_handle(p_idx);

	if (relative_handle.gizmo.is_valid()) {
		// If the gizmo is valid, take the handle name.
		return relative_handle.gizmo->get_handle_name(p_gizmo, relative_handle.idx);
	}

	return "";
}

Variant Components3DGizmoPlugin::get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const {
	// Find the gizmo ID.
	RelativeHandle relative_handle = find_gizmo_by_handle(p_idx);

	if (relative_handle.gizmo.is_valid()) {
		// If the gizmo is valid, take the handle name.
		return relative_handle.gizmo->get_handle_value(p_gizmo, relative_handle.idx);
	}

	return Variant();
}

void Components3DGizmoPlugin::set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
	// Find the gizmo ID.
	RelativeHandle relative_handle = find_gizmo_by_handle(p_idx);

	if (relative_handle.gizmo.is_valid()) {
		// If the gizmo is valid, take the handle name.
		relative_handle.gizmo->set_handle(p_gizmo, relative_handle.idx, p_camera, p_point);
	}
}

void Components3DGizmoPlugin::commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
	RelativeHandle relative_handle = find_gizmo_by_handle(p_idx);

	if (relative_handle.gizmo.is_valid()) {
		// If the gizmo is valid, take the handle name.
		relative_handle.gizmo->commit_handle(p_gizmo, relative_handle.idx, p_restore, p_cancel);
	}
}

RelativeHandle Components3DGizmoPlugin::find_gizmo_by_handle(int p_idx) const {
	if (p_idx < 0) {
		return {};
	}

	int bank = 0;
	Ref<ComponentGizmo> giz;
	for (uint32_t i = 0; i < gizmos.size(); i += 1) {
		giz = gizmos[i];
		if (bank + gizmos[i]->get_handle_count() > p_idx) {
			// The previous gizmo was able to handle this.
			return { giz, p_idx - bank };
		}
		bank += gizmos[i]->get_handle_count();
	}

	return {};
}

void TransformComponentGizmo::init() {
	const Color gizmo_color_x = EDITOR_DEF("editors/3d_gizmos/gizmo_colors/x", Color(0.98, 0.1, 0.1));
	create_material("transform_gizmo_x", gizmo_color_x);

	const Color gizmo_color_y = EDITOR_DEF("editors/3d_gizmos/gizmo_colors/y", Color(0.1, 0.98, 0.1));
	create_material("transform_gizmo_y", gizmo_color_y);

	const Color gizmo_color_z = EDITOR_DEF("editors/3d_gizmos/gizmo_colors/z", Color(0.1, 0.1, 0.98));
	create_material("transform_gizmo_z", gizmo_color_z);
}

void TransformComponentGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());

	if (entity->has_component(transform_component_name) == false) {
		// Nothing to do.
		return;
	}

	{
		const Ref<Material> material = get_material("transform_gizmo_x", p_gizmo);
		Vector<Vector3> segments;
		segments.push_back(Vector3(0.25, 0, 0));
		segments.push_back(Vector3(-0.25, 0, 0));

		segments.push_back(Vector3(0.25, 0, 0));
		segments.push_back(Vector3(0.2, 0.05, 0));
		segments.push_back(Vector3(0.25, 0, 0));
		segments.push_back(Vector3(0.2, -0.05, 0));

		segments.push_back(Vector3(0.25, 0, 0));
		segments.push_back(Vector3(0.2, 0, 0.05));
		segments.push_back(Vector3(0.25, 0, 0));
		segments.push_back(Vector3(0.2, 0, -0.05));

		p_gizmo->add_lines(segments, material);
		p_gizmo->add_collision_segments(segments);
	}

	{
		const Ref<Material> material = get_material("transform_gizmo_y", p_gizmo);
		Vector<Vector3> segments;
		segments.push_back(Vector3(0, 0.25, 0));
		segments.push_back(Vector3(0, -0.25, 0));

		segments.push_back(Vector3(0, 0.25, 0));
		segments.push_back(Vector3(0.05, 0.2, 0));
		segments.push_back(Vector3(0, 0.25, 0));
		segments.push_back(Vector3(-0.05, 0.2, 0));

		segments.push_back(Vector3(0, 0.25, 0));
		segments.push_back(Vector3(0, 0.2, 0.05));
		segments.push_back(Vector3(0, 0.25, 0));
		segments.push_back(Vector3(0, 0.2, -0.05));

		p_gizmo->add_lines(segments, material);
		p_gizmo->add_collision_segments(segments);
	}

	{
		const Ref<Material> material = get_material("transform_gizmo_z", p_gizmo);
		Vector<Vector3> segments;
		segments.push_back(Vector3(0, 0, 0.25));
		segments.push_back(Vector3(0, 0, -0.25));

		segments.push_back(Vector3(0, 0, 0.25));
		segments.push_back(Vector3(0, 0.05, 0.2));
		segments.push_back(Vector3(0, 0, 0.25));
		segments.push_back(Vector3(0, -0.05, 0.2));

		segments.push_back(Vector3(0, 0, 0.25));
		segments.push_back(Vector3(0.05, 0, 0.2));
		segments.push_back(Vector3(0, 0, 0.25));
		segments.push_back(Vector3(-0.05, 0, 0.2));

		p_gizmo->add_lines(segments, material);
		p_gizmo->add_collision_segments(segments);
	}
}

int TransformComponentGizmo::get_handle_count() const {
	return 0;
}

String TransformComponentGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return "";
}

Variant TransformComponentGizmo::get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return Variant();
}

void TransformComponentGizmo::set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
	return;
}

void TransformComponentGizmo::commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
	return;
}

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

void MeshComponentGizmo::EditorMeshData::on_position_update(const Transform &p_new_transform) {
	RenderingServer::get_singleton()->instance_set_transform(instance, p_new_transform);
}

void MeshComponentGizmo::init() {}

void MeshComponentGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());

	RID scenario = entity->get_world_3d()->get_scenario();
	if (entity->has_component(mesh_component_name) == false || scenario == RID()) {
		// No mesh component, remove any editor mesh.
		Ref<ComponentGizmoData> *editor_mesh = entity->get_internal_entity().gizmo_data.lookup_ptr(mesh_component_name);
		if (editor_mesh == nullptr) {
			// Nothing to do.
			return;
		}

		entity->get_internal_entity().gizmo_data.remove(mesh_component_name);

	} else {
		// Mesh data, make sure to add the editor mesh.
		Ref<EditorMeshData> editor_mesh;
		{
			Ref<ComponentGizmoData> *d = entity->get_internal_entity().gizmo_data.lookup_ptr(mesh_component_name);
			if (d == nullptr || d->is_null()) {
				editor_mesh.instance();
				entity->get_internal_entity().gizmo_data.insert(mesh_component_name, editor_mesh);
			} else {
				editor_mesh = *d;
				RenderingServer::get_singleton()->instance_attach_object_instance_id(editor_mesh->instance, entity->get_instance_id());
				RenderingServer::get_singleton()->instance_set_scenario(editor_mesh->instance, scenario);
			}
		}

		// Update the mesh.
		Ref<Mesh> m = entity->get_component_value(mesh_component_name, "mesh");
		editor_mesh->set_mesh(m);

		if (m.is_valid()) {
			Ref<TriangleMesh> tm = m->generate_triangle_mesh();
			if (tm.is_valid()) {
				p_gizmo->add_collision_triangles(tm);
			}
		}

		// Update the transform.
		RenderingServer::get_singleton()->instance_set_transform(editor_mesh->instance, entity->get_global_transform());

		// Update the material.
		// TODO
	}
}

int MeshComponentGizmo::get_handle_count() const {
	return 0;
}

String MeshComponentGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return "";
}

Variant MeshComponentGizmo::get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return Variant();
}

void MeshComponentGizmo::set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
	return;
}

void MeshComponentGizmo::commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
	return;
}
