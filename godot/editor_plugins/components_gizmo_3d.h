#pragma once

#include "../nodes/entity.h"
#include "editor/node_3d_editor_gizmos.h"

class ComponentGizmo;

struct RelativeHandle {
	Ref<ComponentGizmo> gizmo;
	int idx;
};

class Components3DGizmoPlugin : public EditorNode3DGizmoPlugin {
	GDCLASS(Components3DGizmoPlugin, EditorNode3DGizmoPlugin);

	LocalVector<Ref<ComponentGizmo>> gizmos;

public:
	Components3DGizmoPlugin();

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

	RelativeHandle find_gizmo_by_handle(int p_idx) const;
};

class ComponentGizmo : public Reference {
	friend class Components3DGizmoPlugin;

protected:
	Components3DGizmoPlugin *owner = nullptr;
	bool is_init = false;

public:
	virtual void init() = 0;
	virtual void redraw(EditorNode3DGizmo *p_gizmo) = 0;
	virtual int get_handle_count() const = 0;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const = 0;
	virtual Variant get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const = 0;
	virtual void set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) = 0;
	virtual void commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) = 0;

	void create_material(const String &p_name, const Color &p_color, bool p_billboard = false, bool p_on_top = false, bool p_use_vertex_color = false) {
		owner->create_material(p_name, p_color, p_billboard, p_on_top, p_use_vertex_color);
	}

	Ref<StandardMaterial3D> get_material(const String &p_name, const Ref<EditorNode3DGizmo> &p_gizmo = Ref<EditorNode3DGizmo>()) {
		return owner->get_material(p_name, p_gizmo);
	}
};

class TransformComponentGizmo : public ComponentGizmo {
	StringName transform_component_name = "TransformComponent";

public:
	virtual void init() override;
	virtual void redraw(EditorNode3DGizmo *p_gizmo) override;
	virtual int get_handle_count() const override;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual Variant get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual void set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) override;
	virtual void commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) override;
};

class MeshComponentGizmo : public ComponentGizmo {
	StringName mesh_component_name = "MeshComponent";
	StringName transform_component_name = "TransformComponent";
	StringName transform_name = "transform";

	struct EditorMeshData : public ComponentGizmoData {
		RID base;
		RID instance;

		EditorMeshData();
		~EditorMeshData();
		void set_mesh(Ref<Mesh> p_mesh);
	};

public:
	virtual void init() override;
	virtual void redraw(EditorNode3DGizmo *p_gizmo) override;
	virtual int get_handle_count() const override;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual Variant get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual void set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) override;
	virtual void commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) override;
};
