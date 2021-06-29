#pragma once

#include "../nodes/entity.h"
#include "editor/node_3d_editor_gizmos.h"

class ComponentGizmo;

struct RelativeHandle {
	Ref<ComponentGizmo> gizmo;
	int idx;
};

/// Only one `GizmoPlugin` at a time is allowed by Godot.
/// Since the `Entity` may need more than 1 gizmo depending on the `Component`s
/// it has, this `GizmoPlugin` has a mechanism to handle more gizmos at the same
/// time.
class Components3DGizmoPlugin : public EditorNode3DGizmoPlugin {
	GDCLASS(Components3DGizmoPlugin, EditorNode3DGizmoPlugin);

	static Components3DGizmoPlugin *singleton;

	LocalVector<Ref<ComponentGizmo>> gizmos;

public:
	static Components3DGizmoPlugin *get_singleton() { return singleton; }

	Components3DGizmoPlugin();
	~Components3DGizmoPlugin();

	/// Add a component gizmo.
	void add_component_gizmo(Ref<ComponentGizmo> p_gizmo);

	virtual bool has_gizmo(Node3D *p_node) override;
	virtual String get_gizmo_name() const override;
	virtual int get_priority() const override;

	virtual void redraw(EditorNode3DGizmo *p_gizmo) override;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual Variant get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual void set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) override;
	virtual void commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) override;

	RelativeHandle find_gizmo_by_handle(const EditorNode3DGizmo *p_gizmo, int p_idx) const;
};

class ComponentGizmo : public RefCounted {
	friend class Components3DGizmoPlugin;

protected:
	Components3DGizmoPlugin *owner = nullptr;
	bool is_init = false;

public:
	virtual void init() = 0;
	virtual void redraw(EditorNode3DGizmo *p_gizmo) = 0;
	virtual int get_handle_count(const EditorNode3DGizmo *p_gizmo) const = 0;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const = 0;
	virtual Variant get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const = 0;
	virtual void set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) = 0;
	virtual void commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) = 0;

	void create_material(const String &p_name, const Color &p_color, bool p_billboard = false, bool p_on_top = false, bool p_use_vertex_color = false) {
		owner->create_material(p_name, p_color, p_billboard, p_on_top, p_use_vertex_color);
	}

	void create_handle_material(const String &p_name, bool p_billboard = false, const Ref<Texture2D> &p_texture = nullptr) {
		owner->create_handle_material(p_name, p_billboard, p_texture);
	}

	Ref<StandardMaterial3D> get_material(const String &p_name, const Ref<EditorNode3DGizmo> &p_gizmo = Ref<EditorNode3DGizmo>()) {
		return owner->get_material(p_name, p_gizmo);
	}
};
