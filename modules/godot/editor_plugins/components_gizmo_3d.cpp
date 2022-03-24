#include "components_gizmo_3d.h"

#include "../nodes/entity.h"
#include "servers/rendering_server.h"

Components3DGizmoPlugin *Components3DGizmoPlugin::singleton = nullptr;

Components3DGizmoPlugin::Components3DGizmoPlugin() {
	CRASH_COND(singleton != nullptr);
	singleton = this;
}

Components3DGizmoPlugin::~Components3DGizmoPlugin() {
	singleton = nullptr;
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

String Components3DGizmoPlugin::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx, bool p_secondary) const {
	// Find the gizmo ID.
	RelativeHandle relative_handle = find_gizmo_by_handle(p_gizmo, p_idx);

	if (relative_handle.gizmo.is_valid()) {
		// If the gizmo is valid, take the handle name.
		return relative_handle.gizmo->get_handle_name(p_gizmo, relative_handle.idx);
	}

	return "";
}

Variant Components3DGizmoPlugin::get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx, bool p_secondary) const {
	// Find the gizmo ID.
	RelativeHandle relative_handle = find_gizmo_by_handle(p_gizmo, p_idx);

	if (relative_handle.gizmo.is_valid()) {
		// If the gizmo is valid, take the handle name.
		return relative_handle.gizmo->get_handle_value(p_gizmo, relative_handle.idx);
	}

	return Variant();
}

void Components3DGizmoPlugin::set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, bool p_secondary, Camera3D *p_camera, const Point2 &p_point) {
	// Find the gizmo ID.
	RelativeHandle relative_handle = find_gizmo_by_handle(p_gizmo, p_idx);

	if (relative_handle.gizmo.is_valid()) {
		// If the gizmo is valid, take the handle name.
		relative_handle.gizmo->set_handle(p_gizmo, relative_handle.idx, p_camera, p_point);
	}
}

void Components3DGizmoPlugin::commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, bool p_secondary, const Variant &p_restore, bool p_cancel) {
	RelativeHandle relative_handle = find_gizmo_by_handle(p_gizmo, p_idx);

	if (relative_handle.gizmo.is_valid()) {
		// If the gizmo is valid, take the handle name.
		relative_handle.gizmo->commit_handle(p_gizmo, relative_handle.idx, p_restore, p_cancel);
	}
}

RelativeHandle Components3DGizmoPlugin::find_gizmo_by_handle(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	if (p_idx < 0) {
		return {};
	}

	int bank = 0;
	Ref<ComponentGizmo> giz;
	for (uint32_t i = 0; i < gizmos.size(); i += 1) {
		giz = gizmos[i];
		if (bank + gizmos[i]->get_handle_count(p_gizmo) > p_idx) {
			// The previous gizmo was able to handle this.
			return { giz, p_idx - bank };
		}
		bank += gizmos[i]->get_handle_count(p_gizmo);
	}

	return {};
}
