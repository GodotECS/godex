#pragma once

#include "../nodes/entity.h"
#include "components_gizmo_3d.h"

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

		virtual void on_position_update(const Transform3D &p_new_transform) override;
	};

public:
	virtual void init() override;
	virtual void redraw(EditorNode3DGizmo *p_gizmo) override;
	virtual int get_handle_count(const EditorNode3DGizmo *p_gizmo) const override;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual Variant get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual void set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) override;
	virtual void commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) override;
};
