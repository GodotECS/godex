#pragma once

#include "../nodes/entity.h"
#include "components_gizmo_3d.h"

class TransformComponentGizmo : public ComponentGizmo {
	StringName transform_component_name = "TransformComponent";

public:
	virtual void init() override;
	virtual void redraw(EditorNode3DGizmo *p_gizmo) override;
	virtual int get_handle_count(const EditorNode3DGizmo *p_gizmo) const override;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual Variant get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual void set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) override;
	virtual void commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) override;
};
