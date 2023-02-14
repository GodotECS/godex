#include "components_gizmos.h"

#include "debug_utilities.h"
#include "editor/editor_node.h"
#include "editor/editor_settings.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/plugins/node_3d_editor_plugin.h"

void BtBoxGizmo::init() {
	const Color gizmo_color = EDITOR_DEF("editors/3d_gizmos/gizmo_colors/shape", Color(0.5, 0.7, 1));
	create_material("shape_material", gizmo_color);

	const float gizmo_value = gizmo_color.get_v();
	const Color gizmo_color_disabled = Color(gizmo_value, gizmo_value, gizmo_value, 0.65);
	create_material("shape_material_disabled", gizmo_color_disabled);

	create_handle_material("handles");
}

void BtBoxGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);

	if (entity->has_component(box_component_name)) {
		const Ref<Material> material = get_material("shape_material", p_gizmo);
		// const Ref<Material> material_disabled = get_material("shape_material_disabled", p_gizmo);
		Ref<Material> handles_material = get_material("handles");

		Vector3 half_extents = entity->get_component_value(box_component_name, half_extents_name);

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

int BtBoxGizmo::get_handle_count(const EditorNode3DGizmo *p_gizmo) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND_V(entity == nullptr, 0);

	if (entity->has_component(box_component_name)) {
		return 3;
	}

	return 0;
}

String BtBoxGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	if (p_idx == 0) {
		return "half_extents_x";
	} else if (p_idx == 1) {
		return "half_extents_y";
	} else {
		return "half_extents_z";
	}
}

Variant BtBoxGizmo::get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND_V(entity == nullptr, Variant());
	ERR_FAIL_COND_V(entity->has_component(box_component_name) == false, Variant());

	const Vector3 half_extents = entity->get_component_value(box_component_name, half_extents_name, Space::GLOBAL);
	ERR_FAIL_COND_V_MSG((p_idx < 0) || p_idx > 2, Variant(), "The index " + itos(p_idx) + "is invalid. It should go from 0 to 2 (inclusive).");
	return half_extents[p_idx];
}

void BtBoxGizmo::set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(box_component_name) == false);

	Transform3D gt = entity->get_global_transform();
	Transform3D gi = gt.affine_inverse();

	Vector3 ray_from = p_camera->project_ray_origin(p_point);
	Vector3 ray_dir = p_camera->project_ray_normal(p_point);
	Vector3 sg[2] = { gi.xform(ray_from), gi.xform(ray_from + ray_dir * 4096) };

	// Extract the Handle value
	Vector3 axis;
	axis[p_idx] = 1.0;
	Vector3 ra, rb;
	Geometry3D::get_closest_points_between_segments(Vector3(), axis * 4096, sg[0], sg[1], ra, rb);
	float d = ra[p_idx];
	if (Node3DEditor::get_singleton()->is_snap_enabled()) {
		d = Math::snapped(d, Node3DEditor::get_singleton()->get_translate_snap());
	}

	if (d < 0.001) {
		d = 0.001;
	}

	Vector3 half_extents = entity->get_component_value(box_component_name, half_extents_name);
	half_extents[p_idx] = d;

	entity->set_component_value(box_component_name, half_extents_name, half_extents);
}

void BtBoxGizmo::commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(box_component_name) == false);

	Vector3 half_extents = entity->get_component_value(box_component_name, half_extents_name);

	if (p_cancel) {
		entity->set_component_value(box_component_name, half_extents_name, p_restore);
	} else {
		EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
		ur->create_action(TTR("Change Shape Box Half Extent"));
		ur->add_do_method(entity, "set_component_value", box_component_name, half_extents_name, half_extents);
		Vector3 restore = half_extents;
		restore[p_idx] = p_restore;
		ur->add_undo_method(entity, "set_component_value", box_component_name, half_extents_name, restore);
		ur->commit_action();
	}
}

void BtSphereGizmo::init() {
	// No need to register the materials, everything we need is already
	// registered by the box gizmo.
}

void BtSphereGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);

	if (entity->has_component(sphere_component_name)) {
		const Ref<Material> material = get_material("shape_material", p_gizmo);
		// const Ref<Material> material_disabled = get_material("shape_material_disabled", p_gizmo);
		Ref<Material> handles_material = get_material("handles");

		const real_t radius = entity->get_component_value(sphere_component_name, radius_name);

		Vector<Vector3> points;

		for (int i = 0; i <= 360; i++) {
			float ra = Math::deg_to_rad((float)i);
			float rb = Math::deg_to_rad((float)i + 1);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			points.push_back(Vector3(a.x, 0, a.y));
			points.push_back(Vector3(b.x, 0, b.y));
			points.push_back(Vector3(0, a.x, a.y));
			points.push_back(Vector3(0, b.x, b.y));
			points.push_back(Vector3(a.x, a.y, 0));
			points.push_back(Vector3(b.x, b.y, 0));
		}

		Vector<Vector3> collision_segments;

		for (int i = 0; i < 64; i++) {
			float ra = i * (Math_TAU / 64.0);
			float rb = (i + 1) * (Math_TAU / 64.0);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			collision_segments.push_back(Vector3(a.x, 0, a.y));
			collision_segments.push_back(Vector3(b.x, 0, b.y));
			collision_segments.push_back(Vector3(0, a.x, a.y));
			collision_segments.push_back(Vector3(0, b.x, b.y));
			collision_segments.push_back(Vector3(a.x, a.y, 0));
			collision_segments.push_back(Vector3(b.x, b.y, 0));
		}

		p_gizmo->add_lines(points, material);
		p_gizmo->add_collision_segments(collision_segments);
		Vector<Vector3> handles;
		handles.push_back(Vector3(radius, 0, 0));
		p_gizmo->add_handles(handles, handles_material);
	}
}

int BtSphereGizmo::get_handle_count(const EditorNode3DGizmo *p_gizmo) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND_V(entity == nullptr, 0);

	if (entity->has_component(sphere_component_name)) {
		return 1;
	}

	return 0;
}

String BtSphereGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return "radius";
}

Variant BtSphereGizmo::get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND_V(entity == nullptr, Variant());
	ERR_FAIL_COND_V(entity->has_component(sphere_component_name) == false, Variant());

	return entity->get_component_value(sphere_component_name, radius_name);
}

void BtSphereGizmo::set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(sphere_component_name) == false);

	Transform3D gt = entity->get_global_transform();
	Transform3D gi = gt.affine_inverse();

	Vector3 ray_from = p_camera->project_ray_origin(p_point);
	Vector3 ray_dir = p_camera->project_ray_normal(p_point);
	Vector3 sg[2] = { gi.xform(ray_from), gi.xform(ray_from + ray_dir * 4096) };

	// Extract the Handle value
	Vector3 ra, rb;
	Geometry3D::get_closest_points_between_segments(Vector3(), Vector3(4096, 0, 0), sg[0], sg[1], ra, rb);
	float d = ra.x;
	if (Node3DEditor::get_singleton()->is_snap_enabled()) {
		d = Math::snapped(d, Node3DEditor::get_singleton()->get_translate_snap());
	}

	if (d < 0.001) {
		d = 0.001;
	}

	entity->set_component_value(sphere_component_name, radius_name, d);
}

void BtSphereGizmo::commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(sphere_component_name) == false);

	const real_t radius = entity->get_component_value(sphere_component_name, radius_name);

	if (p_cancel) {
		entity->set_component_value(sphere_component_name, radius_name, p_restore);
	} else {
		EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
		ur->create_action(TTR("Change Shape Sphere Radius"));
		ur->add_do_method(entity, "set_component_value", sphere_component_name, radius_name, radius);
		ur->add_undo_method(entity, "set_component_value", sphere_component_name, radius_name, p_restore);
		ur->commit_action();
	}
}

void BtCapsuleGizmo::init() {
	// No need to register the materials, everything we need is already
	// registered by the box gizmo.
}

void BtCapsuleGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);

	if (entity->has_component(capsule_component_name)) {
		const Ref<Material> material = get_material("shape_material", p_gizmo);
		// const Ref<Material> material_disabled = get_material("shape_material_disabled", p_gizmo);
		Ref<Material> handles_material = get_material("handles");

		const real_t radius = entity->get_component_value(capsule_component_name, radius_name);
		const real_t height = entity->get_component_value(capsule_component_name, height_name);

		Vector<Vector3> points;

		Vector3 d(0, height * 0.5, 0);
		for (int i = 0; i < 360; i++) {
			float ra = Math::deg_to_rad((float)i);
			float rb = Math::deg_to_rad((float)i + 1);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			points.push_back(Vector3(a.x, 0, a.y) + d);
			points.push_back(Vector3(b.x, 0, b.y) + d);

			points.push_back(Vector3(a.x, 0, a.y) - d);
			points.push_back(Vector3(b.x, 0, b.y) - d);

			if (i % 90 == 0) {
				points.push_back(Vector3(a.x, 0, a.y) + d);
				points.push_back(Vector3(a.x, 0, a.y) - d);
			}

			Vector3 dud = i < 180 ? d : -d;

			points.push_back(Vector3(0, a.x, a.y) + dud);
			points.push_back(Vector3(0, b.x, b.y) + dud);
			points.push_back(Vector3(a.y, a.x, 0) + dud);
			points.push_back(Vector3(b.y, b.x, 0) + dud);
		}

		p_gizmo->add_lines(points, material);

		Vector<Vector3> collision_segments;

		for (int i = 0; i < 64; i++) {
			float ra = i * (Math_TAU / 64.0);
			float rb = (i + 1) * (Math_TAU / 64.0);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			collision_segments.push_back(Vector3(a.x, 0, a.y) + d);
			collision_segments.push_back(Vector3(b.x, 0, b.y) + d);

			collision_segments.push_back(Vector3(a.x, 0, a.y) - d);
			collision_segments.push_back(Vector3(b.x, 0, b.y) - d);

			if (i % 16 == 0) {
				collision_segments.push_back(Vector3(a.x, 0, a.y) + d);
				collision_segments.push_back(Vector3(a.x, 0, a.y) - d);
			}

			Vector3 dud = i < 32 ? d : -d;

			collision_segments.push_back(Vector3(0, a.x, a.y) + dud);
			collision_segments.push_back(Vector3(0, b.x, b.y) + dud);
			collision_segments.push_back(Vector3(a.y, a.x, 0) + dud);
			collision_segments.push_back(Vector3(b.y, b.x, 0) + dud);
		}

		p_gizmo->add_collision_segments(collision_segments);

		Vector<Vector3> handles;
		handles.push_back(Vector3(radius, 0, 0));
		handles.push_back(Vector3(0, height * 0.5 + radius, 0));
		p_gizmo->add_handles(handles, handles_material);
	}
}

int BtCapsuleGizmo::get_handle_count(const EditorNode3DGizmo *p_gizmo) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND_V(entity == nullptr, 0);

	if (entity->has_component(capsule_component_name)) {
		return 2;
	}

	return 0;
}

String BtCapsuleGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	if (p_idx == 0) {
		return "radius";
	} else {
		return "height";
	}
}

Variant BtCapsuleGizmo::get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND_V(entity == nullptr, Variant());
	ERR_FAIL_COND_V(entity->has_component(capsule_component_name) == false, Variant());

	if (p_idx == 0) {
		return entity->get_component_value(capsule_component_name, radius_name);
	} else {
		return entity->get_component_value(capsule_component_name, height_name);
	}
}

void BtCapsuleGizmo::set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(capsule_component_name) == false);

	Transform3D gt = entity->get_global_transform();
	Transform3D gi = gt.affine_inverse();

	Vector3 ray_from = p_camera->project_ray_origin(p_point);
	Vector3 ray_dir = p_camera->project_ray_normal(p_point);
	Vector3 sg[2] = { gi.xform(ray_from), gi.xform(ray_from + ray_dir * 4096) };

	// Extract the Handle value
	Vector3 axis;
	axis[p_idx == 0 ? 0 : 1] = 1.0;
	Vector3 ra, rb;
	Geometry3D::get_closest_points_between_segments(Vector3(), axis * 4096, sg[0], sg[1], ra, rb);
	float d = axis.dot(ra);
	if (p_idx == 1) {
		const real_t radius = entity->get_component_value(capsule_component_name, radius_name);
		d -= radius;
	}

	if (Node3DEditor::get_singleton()->is_snap_enabled()) {
		d = Math::snapped(d, Node3DEditor::get_singleton()->get_translate_snap());
	}

	if (d < 0.001) {
		d = 0.001;
	}

	if (p_idx == 0) {
		entity->set_component_value(capsule_component_name, radius_name, d);
	} else {
		entity->set_component_value(capsule_component_name, height_name, d * 2.0);
	}
}

void BtCapsuleGizmo::commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(capsule_component_name) == false);

	const real_t v = entity->get_component_value(capsule_component_name, p_idx == 0 ? radius_name : height_name);

	if (p_cancel) {
		entity->set_component_value(capsule_component_name, p_idx == 0 ? radius_name : height_name, p_restore);
	} else {
		EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
		ur->create_action(TTR("Change Shape Capsule Radius"));
		ur->add_do_method(entity, "set_component_value", capsule_component_name, p_idx == 0 ? radius_name : height_name, v);
		ur->add_undo_method(entity, "set_component_value", capsule_component_name, p_idx == 0 ? radius_name : height_name, p_restore);
		ur->commit_action();
	}
}

void BtConeGizmo::init() {
	// No need to register the materials, everything we need is already
	// registered by the box gizmo.
}

void BtConeGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);

	if (entity->has_component(cone_component_name)) {
		const Ref<Material> material = get_material("shape_material", p_gizmo);
		// const Ref<Material> material_disabled = get_material("shape_material_disabled", p_gizmo);
		Ref<Material> handles_material = get_material("handles");

		const real_t radius = entity->get_component_value(cone_component_name, radius_name);
		const real_t height = entity->get_component_value(cone_component_name, height_name);

		Vector<Vector3> points;

		Vector3 d(0, height * 0.5, 0);
		for (int i = 0; i < 360; i++) {
			float ra = Math::deg_to_rad((float)i);
			float rb = Math::deg_to_rad((float)i + 1);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			points.push_back(Vector3(a.x, 0, a.y) - d);
			points.push_back(Vector3(b.x, 0, b.y) - d);

			if (i % 90 == 0) {
				points.push_back(Vector3(a.x, 0, a.y) - d);
				points.push_back(d);
			}
		}

		p_gizmo->add_lines(points, material);

		Vector<Vector3> collision_segments;

		for (int i = 0; i < 64; i++) {
			float ra = i * (Math_TAU / 64.0);
			float rb = (i + 1) * (Math_TAU / 64.0);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			collision_segments.push_back(Vector3(a.x, 0, a.y) - d);
			collision_segments.push_back(Vector3(b.x, 0, b.y) - d);

			if (i % 16 == 0) {
				collision_segments.push_back(Vector3(a.x, 0, a.y) - d);
				collision_segments.push_back(d);
			}
		}

		p_gizmo->add_collision_segments(collision_segments);

		Vector<Vector3> handles;
		handles.push_back(Vector3(radius, -height * 0.5, 0));
		handles.push_back(Vector3(0, height * 0.5, 0));
		p_gizmo->add_handles(handles, handles_material);
	}
}

int BtConeGizmo::get_handle_count(const EditorNode3DGizmo *p_gizmo) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND_V(entity == nullptr, 0);

	if (entity->has_component(cone_component_name)) {
		return 2;
	}

	return 0;
}

String BtConeGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	if (p_idx == 0) {
		return "radius";
	} else {
		return "height";
	}
}

Variant BtConeGizmo::get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND_V(entity == nullptr, Variant());
	ERR_FAIL_COND_V(entity->has_component(cone_component_name) == false, Variant());

	if (p_idx == 0) {
		return entity->get_component_value(cone_component_name, radius_name);
	} else {
		return entity->get_component_value(cone_component_name, height_name);
	}
}

void BtConeGizmo::set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(cone_component_name) == false);

	Transform3D gt = entity->get_global_transform();
	Transform3D gi = gt.affine_inverse();

	Vector3 ray_from = p_camera->project_ray_origin(p_point);
	Vector3 ray_dir = p_camera->project_ray_normal(p_point);
	Vector3 sg[2] = { gi.xform(ray_from), gi.xform(ray_from + ray_dir * 4096) };

	// Extract the Handle value
	Vector3 axis;
	axis[p_idx == 0 ? 0 : 1] = 1.0;
	Vector3 ra, rb;
	Geometry3D::get_closest_points_between_segments(Vector3(), axis * 4096, sg[0], sg[1], ra, rb);
	float d = axis.dot(ra);

	if (Node3DEditor::get_singleton()->is_snap_enabled()) {
		d = Math::snapped(d, Node3DEditor::get_singleton()->get_translate_snap());
	}

	if (d < 0.001) {
		d = 0.001;
	}

	if (p_idx == 0) {
		entity->set_component_value(cone_component_name, radius_name, d);
	} else {
		entity->set_component_value(cone_component_name, height_name, d * 2.0);
	}
}

void BtConeGizmo::commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(cone_component_name) == false);

	const real_t v = entity->get_component_value(cone_component_name, p_idx == 0 ? radius_name : height_name);

	if (p_cancel) {
		entity->set_component_value(cone_component_name, p_idx == 0 ? radius_name : height_name, p_restore);
	} else {
		EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
		ur->create_action(TTR("Change Shape Cone Radius"));
		ur->add_do_method(entity, "set_component_value", cone_component_name, p_idx == 0 ? radius_name : height_name, v);
		ur->add_undo_method(entity, "set_component_value", cone_component_name, p_idx == 0 ? radius_name : height_name, p_restore);
		ur->commit_action();
	}
}

void BtCylinderGizmo::init() {
	// No need to register the materials, everything we need is already
	// registered by the box gizmo.
}

void BtCylinderGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);

	if (entity->has_component(cylinder_component_name)) {
		const Ref<Material> material = get_material("shape_material", p_gizmo);
		// const Ref<Material> material_disabled = get_material("shape_material_disabled", p_gizmo);
		Ref<Material> handles_material = get_material("handles");

		const real_t radius = entity->get_component_value(cylinder_component_name, radius_name);
		const real_t height = entity->get_component_value(cylinder_component_name, height_name);

		Vector<Vector3> points;

		Vector3 d(0, height * 0.5, 0);
		for (int i = 0; i < 360; i++) {
			float ra = Math::deg_to_rad((float)i);
			float rb = Math::deg_to_rad((float)i + 1);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			points.push_back(Vector3(a.x, 0, a.y) + d);
			points.push_back(Vector3(b.x, 0, b.y) + d);

			points.push_back(Vector3(a.x, 0, a.y) - d);
			points.push_back(Vector3(b.x, 0, b.y) - d);

			if (i % 90 == 0) {
				points.push_back(Vector3(a.x, 0, a.y) + d);
				points.push_back(Vector3(a.x, 0, a.y) - d);

				points.push_back(Vector3(a.x, 0, a.y) + d);
				points.push_back(d);

				points.push_back(Vector3(a.x, 0, a.y) - d);
				points.push_back(-d);
			}
		}

		p_gizmo->add_lines(points, material);

		Vector<Vector3> collision_segments;

		for (int i = 0; i < 64; i++) {
			float ra = i * (Math_TAU / 64.0);
			float rb = (i + 1) * (Math_TAU / 64.0);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			collision_segments.push_back(Vector3(a.x, 0, a.y) + d);
			collision_segments.push_back(Vector3(b.x, 0, b.y) + d);

			collision_segments.push_back(Vector3(a.x, 0, a.y) - d);
			collision_segments.push_back(Vector3(b.x, 0, b.y) - d);

			if (i % 16 == 0) {
				collision_segments.push_back(Vector3(a.x, 0, a.y) + d);
				collision_segments.push_back(Vector3(a.x, 0, a.y) - d);

				collision_segments.push_back(Vector3(a.x, 0, a.y) + d);
				collision_segments.push_back(d);

				collision_segments.push_back(Vector3(a.x, 0, a.y) - d);
				collision_segments.push_back(-d);
			}
		}

		p_gizmo->add_collision_segments(collision_segments);

		Vector<Vector3> handles;
		handles.push_back(Vector3(radius, 0, 0));
		handles.push_back(Vector3(0, height * 0.5, 0));
		p_gizmo->add_handles(handles, handles_material);
	}
}

int BtCylinderGizmo::get_handle_count(const EditorNode3DGizmo *p_gizmo) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND_V(entity == nullptr, 0);

	if (entity->has_component(cylinder_component_name)) {
		return 2;
	}

	return 0;
}

String BtCylinderGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	if (p_idx == 0) {
		return "radius";
	} else {
		return "height";
	}
}

Variant BtCylinderGizmo::get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND_V(entity == nullptr, Variant());
	ERR_FAIL_COND_V(entity->has_component(cylinder_component_name) == false, Variant());

	if (p_idx == 0) {
		return entity->get_component_value(cylinder_component_name, radius_name);
	} else {
		return entity->get_component_value(cylinder_component_name, height_name);
	}
}

void BtCylinderGizmo::set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(cylinder_component_name) == false);

	Transform3D gt = entity->get_global_transform();
	Transform3D gi = gt.affine_inverse();

	Vector3 ray_from = p_camera->project_ray_origin(p_point);
	Vector3 ray_dir = p_camera->project_ray_normal(p_point);
	Vector3 sg[2] = { gi.xform(ray_from), gi.xform(ray_from + ray_dir * 4096) };

	// Extract the Handle value
	Vector3 axis;
	axis[p_idx == 0 ? 0 : 1] = 1.0;
	Vector3 ra, rb;
	Geometry3D::get_closest_points_between_segments(Vector3(), axis * 4096, sg[0], sg[1], ra, rb);
	float d = axis.dot(ra);
	if (Node3DEditor::get_singleton()->is_snap_enabled()) {
		d = Math::snapped(d, Node3DEditor::get_singleton()->get_translate_snap());
	}

	if (d < 0.001) {
		d = 0.001;
	}

	if (p_idx == 0) {
		entity->set_component_value(cylinder_component_name, radius_name, d);
	} else {
		entity->set_component_value(cylinder_component_name, height_name, d * 2.0);
	}
}

void BtCylinderGizmo::commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(cylinder_component_name) == false);

	const real_t v = entity->get_component_value(cylinder_component_name, p_idx == 0 ? radius_name : height_name);

	if (p_cancel) {
		entity->set_component_value(cylinder_component_name, p_idx == 0 ? radius_name : height_name, p_restore);
	} else {
		EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
		ur->create_action(TTR("Change Shape Capsule Radius"));
		ur->add_do_method(entity, "set_component_value", cylinder_component_name, p_idx == 0 ? radius_name : height_name, v);
		ur->add_undo_method(entity, "set_component_value", cylinder_component_name, p_idx == 0 ? radius_name : height_name, p_restore);
		ur->commit_action();
	}
}

class DebugMeshComponentGizmoData : public ComponentGizmoData {
public:
	// Cache data.
	Ref<ArrayMesh> mesh;
};

void BtConvexGizmo::init() {
	// No need to register the materials, everything we need is already
	// registered by the box gizmo.
}

void BtConvexGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);

	if (entity->has_component(convex_component_name)) {
		const Ref<Material> material = get_material("shape_material", p_gizmo);

		// Check the cache.
		Ref<ArrayMesh> mesh;
		Ref<ComponentGizmoData> *d = entity->get_internal_entity().gizmo_data.lookup_ptr(convex_component_name);
		if (d) {
			Ref<DebugMeshComponentGizmoData> td = *d;
			if (td.is_valid()) {
				mesh = td->mesh;
			}
		}

		if (mesh.is_null()) {
			const Vector<Vector3> points = entity->get_component_value(convex_component_name, points_name);
			mesh = generate_mesh_from_points(points);

			// Cache this
			entity->get_internal_entity().gizmo_data.insert(convex_component_name, mesh);
		}

		p_gizmo->add_mesh(mesh, material);
	}
}

int BtConvexGizmo::get_handle_count(const EditorNode3DGizmo *p_gizmo) const {
	return 0;
}

String BtConvexGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return "";
}

Variant BtConvexGizmo::get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return Variant();
}

void BtConvexGizmo::set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
}

void BtConvexGizmo::commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
}

void BtTrimeshGizmo::init() {
	// No need to register the materials, everything we need is already
	// registered by the box gizmo.
}

void BtTrimeshGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);

	if (entity->has_component(trimesh_component_name)) {
		const Ref<Material> material = get_material("shape_material", p_gizmo);

		// Check the cache.
		Ref<ArrayMesh> mesh;
		Ref<ComponentGizmoData> *d = entity->get_internal_entity().gizmo_data.lookup_ptr(trimesh_component_name);
		if (d) {
			Ref<DebugMeshComponentGizmoData> td = *d;
			if (td.is_valid()) {
				mesh = td->mesh;
			}
		}

		if (mesh.is_null()) {
			const Vector<Vector3> faces = entity->get_component_value(trimesh_component_name, faces_name);
			mesh = generate_mesh_from_faces(faces);

			// Cache this
			entity->get_internal_entity().gizmo_data.insert(trimesh_component_name, mesh);
		}

		p_gizmo->add_mesh(mesh, material);
	}
}

int BtTrimeshGizmo::get_handle_count(const EditorNode3DGizmo *p_gizmo) const {
	return 0;
}

String BtTrimeshGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return "";
}

Variant BtTrimeshGizmo::get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	return Variant();
}

void BtTrimeshGizmo::set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
}

void BtTrimeshGizmo::commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
}

void BtPawnGizmo::init() {
	const Color gizmo_color = EDITOR_DEF("editors/3d_gizmos/gizmo_colors/pawn_stance0_shape", Color(1.0, 0.5, 1));
	create_material("pawn_stance0_shape_material", gizmo_color);

	const Color gizmo_color2 = EDITOR_DEF("editors/3d_gizmos/gizmo_colors/pawn_stance1_shape", Color(0.5, 0.5, 1));
	create_material("pawn_stance1_shape_material", gizmo_color2);
}

void BtPawnGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);

	if (entity->has_component(pawn_component_name)) {
		const real_t stance0_pawn_height = entity->get_component_value(pawn_component_name, stance0_height_name);
		const real_t stance1_pawn_height = entity->get_component_value(pawn_component_name, stance1_height_name);

		const real_t ground_position =
				MAX(stance0_pawn_height, stance1_pawn_height) / -2.0;

		{
			const Ref<Material> material = get_material("pawn_stance0_shape_material", p_gizmo);

			const real_t pawn_height = stance0_pawn_height;
			const real_t radius = entity->get_component_value(pawn_component_name, stance0_radius_name);
			const real_t height = pawn_height - radius * 2.0;

			Vector3 offset;
			offset.y = ground_position + (pawn_height / 2.0);

			redraw_capsule(p_gizmo, material, height, radius, offset);
		}

		{
			const Ref<Material> material = get_material("pawn_stance1_shape_material", p_gizmo);

			const real_t pawn_height = stance1_pawn_height;
			const real_t radius = entity->get_component_value(pawn_component_name, stance1_radius_name);
			const real_t height = pawn_height - radius * 2.0;

			Vector3 offset;
			offset.y = ground_position + (pawn_height / 2.0);

			redraw_capsule(p_gizmo, material, height, radius, offset);
		}
	}
}

void BtPawnGizmo::redraw_capsule(EditorNode3DGizmo *p_gizmo, const Ref<Material> p_material, real_t p_height, real_t p_radius, const Vector3 &p_offset) {
	Ref<Material> handles_material = get_material("handles");

	Vector<Vector3> points;

	Vector3 d(0, p_height * 0.5, 0);
	for (int i = 0; i < 360; i++) {
		float ra = Math::deg_to_rad((float)i);
		float rb = Math::deg_to_rad((float)i + 1);
		Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * p_radius;
		Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * p_radius;

		points.push_back(Vector3(a.x, 0, a.y) + d + p_offset);
		points.push_back(Vector3(b.x, 0, b.y) + d + p_offset);

		points.push_back(Vector3(a.x, 0, a.y) - d + p_offset);
		points.push_back(Vector3(b.x, 0, b.y) - d + p_offset);

		if (i % 90 == 0) {
			points.push_back(Vector3(a.x, 0, a.y) + d + p_offset);
			points.push_back(Vector3(a.x, 0, a.y) - d + p_offset);
		}

		Vector3 dud = i < 180 ? d : -d;

		points.push_back(Vector3(0, a.x, a.y) + dud + p_offset);
		points.push_back(Vector3(0, b.x, b.y) + dud + p_offset);
		points.push_back(Vector3(a.y, a.x, 0) + dud + p_offset);
		points.push_back(Vector3(b.y, b.x, 0) + dud + p_offset);
	}

	p_gizmo->add_lines(points, p_material);

	Vector<Vector3> collision_segments;

	for (int i = 0; i < 64; i++) {
		float ra = i * (Math_TAU / 64.0);
		float rb = (i + 1) * (Math_TAU / 64.0);
		Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * p_radius;
		Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * p_radius;

		collision_segments.push_back(Vector3(a.x, 0, a.y) + d + p_offset);
		collision_segments.push_back(Vector3(b.x, 0, b.y) + d + p_offset);

		collision_segments.push_back(Vector3(a.x, 0, a.y) - d + p_offset);
		collision_segments.push_back(Vector3(b.x, 0, b.y) - d + p_offset);

		if (i % 16 == 0) {
			collision_segments.push_back(Vector3(a.x, 0, a.y) + d + p_offset);
			collision_segments.push_back(Vector3(a.x, 0, a.y) - d + p_offset);
		}

		Vector3 dud = i < 32 ? d : -d;

		collision_segments.push_back(Vector3(0, a.x, a.y) + dud + p_offset);
		collision_segments.push_back(Vector3(0, b.x, b.y) + dud + p_offset);
		collision_segments.push_back(Vector3(a.y, a.x, 0) + dud + p_offset);
		collision_segments.push_back(Vector3(b.y, b.x, 0) + dud + p_offset);
	}

	p_gizmo->add_collision_segments(collision_segments);

	Vector<Vector3> handles;
	handles.push_back(Vector3(p_radius, 0, 0) + p_offset);
	handles.push_back(Vector3(0, p_height * 0.5 + p_radius, 0) + p_offset);

	p_gizmo->add_handles(handles, handles_material);
}

int BtPawnGizmo::get_handle_count(const EditorNode3DGizmo *p_gizmo) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND_V(entity == nullptr, 0);

	if (entity->has_component(pawn_component_name)) {
		return 4;
	}

	return 0;
}

String BtPawnGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	if (p_idx == 0) {
		return "stance0_radius";
	} else if (p_idx == 1) {
		return "stance0_height";
	} else if (p_idx == 2) {
		return "stance1_radius";
	} else {
		return "stance1_height";
	}
}

Variant BtPawnGizmo::get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND_V(entity == nullptr, Variant());
	ERR_FAIL_COND_V(entity->has_component(pawn_component_name) == false, Variant());

	if (p_idx == 0) {
		return entity->get_component_value(pawn_component_name, stance0_radius_name);
	} else if (p_idx == 1) {
		return entity->get_component_value(pawn_component_name, stance0_height_name);
	} else if (p_idx == 2) {
		return entity->get_component_value(pawn_component_name, stance1_radius_name);
	} else {
		return entity->get_component_value(pawn_component_name, stance1_height_name);
	}
}

void BtPawnGizmo::set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(pawn_component_name) == false);

	Transform3D gt = entity->get_global_transform();
	Transform3D gi = gt.affine_inverse();

	Vector3 ray_from = p_camera->project_ray_origin(p_point);
	Vector3 ray_dir = p_camera->project_ray_normal(p_point);
	Vector3 sg[2] = { gi.xform(ray_from), gi.xform(ray_from + ray_dir * 4096) };

	// Extract the Handle value
	Vector3 axis;
	axis[p_idx % 2 == 0 ? 0 : 1] = 1.0;
	Vector3 ra, rb;
	float d;

	if (p_idx == 1 || p_idx == 3) {
		const real_t stance0_pawn_height = entity->get_component_value(pawn_component_name, stance0_height_name);
		const real_t stance1_pawn_height = entity->get_component_value(pawn_component_name, stance1_height_name);
		const real_t ground_position =
				MAX(stance0_pawn_height, stance1_pawn_height) / -2.0;

		Geometry3D::get_closest_points_between_segments(axis * ground_position, axis * 4096, sg[0], sg[1], ra, rb);
		d = axis.dot(ra);

		// Take the height of the pawn computing the distance from the ground position.
		d -= ground_position;
	} else {
		Geometry3D::get_closest_points_between_segments(Vector3(), axis * 4096, sg[0], sg[1], ra, rb);
		d = axis.dot(ra);
	}

	if (Node3DEditor::get_singleton()->is_snap_enabled()) {
		d = Math::snapped(d, Node3DEditor::get_singleton()->get_translate_snap());
	}

	if (d < 0.001) {
		d = 0.001;
	}

	if (p_idx == 0) {
		entity->set_component_value(pawn_component_name, stance0_radius_name, d);
	} else if (p_idx == 1) {
		entity->set_component_value(pawn_component_name, stance0_height_name, d);
	} else if (p_idx == 2) {
		entity->set_component_value(pawn_component_name, stance1_radius_name, d);
	} else {
		entity->set_component_value(pawn_component_name, stance1_height_name, d);
	}
}

void BtPawnGizmo::commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_node_3d());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(pawn_component_name) == false);

	StringName prop_name;
	if (p_idx == 0) {
		prop_name = stance0_radius_name;
	} else if (p_idx == 1) {
		prop_name = stance0_height_name;
	} else if (p_idx == 2) {
		prop_name = stance1_radius_name;
	} else {
		prop_name = stance1_height_name;
	}

	const real_t v = entity->get_component_value(pawn_component_name, prop_name);

	if (p_cancel) {
		entity->set_component_value(pawn_component_name, prop_name, p_restore);
	} else {
		EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
		ur->create_action(TTR("Change Shape Capsule Radius"));
		ur->add_do_method(entity, "set_component_value", pawn_component_name, prop_name, v);
		ur->add_undo_method(entity, "set_component_value", pawn_component_name, prop_name, p_restore);
		ur->commit_action();
	}
}
