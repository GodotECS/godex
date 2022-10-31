#include "components_transform_gizmo_3d.h"

#include "../nodes/entity.h"
#include "editor/editor_settings.h"
#include "editor/plugins/node_3d_editor_plugin.h"

void TransformComponentGizmo::init() {
	const Color gizmo_color_x = EDITOR_DEF("editors/3d_gizmos/gizmo_colors/x", Color(0.98, 0.1, 0.1));
	create_material("transform_gizmo_x", gizmo_color_x);

	const Color gizmo_color_y = EDITOR_DEF("editors/3d_gizmos/gizmo_colors/y", Color(0.1, 0.98, 0.1));
	create_material("transform_gizmo_y", gizmo_color_y);

	const Color gizmo_color_z = EDITOR_DEF("editors/3d_gizmos/gizmo_colors/z", Color(0.1, 0.1, 0.98));
	create_material("transform_gizmo_z", gizmo_color_z);
}

void TransformComponentGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());

	if (entity->has_component(transform_component_name) == false) {
		// Nothing to do.
		return;
	}

	if (p_gizmo->is_selected() == false) {
		// No gizmo when the entity is not selected in editor.
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

int TransformComponentGizmo::get_handle_count(const EditorNode3DGizmo *p_gizmo) const {
	return 0;
}

String TransformComponentGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return "";
}

Variant TransformComponentGizmo::get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return Variant();
}

void TransformComponentGizmo::set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
	return;
}

void TransformComponentGizmo::commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
	return;
}
