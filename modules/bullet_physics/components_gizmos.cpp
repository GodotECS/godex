#include "components_gizmos.h"

#include "../godot/nodes/entity.h"

void BtShapeComponentsGizmo::init() {
}

void BtShapeComponentsGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());

	const Ref<Material> material = get_material("shape_material", p_gizmo);
	//const Ref<Material> material_disabled = get_material("shape_material_disabled", p_gizmo);
	Ref<Material> handles_material = get_material("handles");

	// ---------------------------------------------------------------------- Box
	if (entity->has_component(box_component_name)) {
		const Vector3 half_extents = entity->get_internal_entity().get_component_value(box_component_name, "half_extents", Space::GLOBAL);

		Vector<Vector3> lines;
		AABB aabb;
		aabb.position = -half_extents;
		aabb.size = half_extents * 2.0;

		for (int i = 0; i < 12; i++) {
			Vector3 a, b;
			aabb.get_edge(i, a, b);
			lines.push_back(a);
			lines.push_back(b);
		}

		Vector<Vector3> handles;

		for (int i = 0; i < 3; i++) {
			Vector3 ax;
			ax[i] = half_extents[i];
			handles.push_back(ax);
		}

		p_gizmo->add_lines(lines, material);
		p_gizmo->add_collision_segments(lines);
		p_gizmo->add_handles(handles, handles_material);
	}
}

int BtShapeComponentsGizmo::get_handle_count() const {
	return 0;
}

String BtShapeComponentsGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return "";
}

Variant BtShapeComponentsGizmo::get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return Variant();
}

void BtShapeComponentsGizmo::set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
	return;
}

void BtShapeComponentsGizmo::commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
	return;
}
