#pragma once

#include "../godot/editor_plugins/components_gizmo_3d.h"

class BtShapeBoxGizmo : public ComponentGizmo {
	StringName box_component_name = "BtShapeBox";
	StringName half_extents_name = "half_extents";

public:
	virtual void init() override;
	virtual void redraw(EditorNode3DGizmo *p_gizmo) override;
	virtual int get_handle_count(const EditorNode3DGizmo *p_gizmo) const override;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual Variant get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual void set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) override;
	virtual void commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) override;
};

class BtShapeSphereGizmo : public ComponentGizmo {
	StringName sphere_component_name = "BtShapeSphere";
	StringName radius_name = "radius";

public:
	virtual void init() override;
	virtual void redraw(EditorNode3DGizmo *p_gizmo) override;
	virtual int get_handle_count(const EditorNode3DGizmo *p_gizmo) const override;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual Variant get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual void set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) override;
	virtual void commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) override;
};

class BtShapeCapsuleGizmo : public ComponentGizmo {
	StringName capsule_component_name = "BtShapeCapsule";
	StringName radius_name = "radius";
	StringName height_name = "height";

public:
	virtual void init() override;
	virtual void redraw(EditorNode3DGizmo *p_gizmo) override;
	virtual int get_handle_count(const EditorNode3DGizmo *p_gizmo) const override;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual Variant get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual void set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) override;
	virtual void commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) override;
};

class BtShapeConeGizmo : public ComponentGizmo {
	StringName cone_component_name = "BtShapeCone";
	StringName radius_name = "radius";
	StringName height_name = "height";

public:
	virtual void init() override;
	virtual void redraw(EditorNode3DGizmo *p_gizmo) override;
	virtual int get_handle_count(const EditorNode3DGizmo *p_gizmo) const override;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual Variant get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual void set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) override;
	virtual void commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) override;
};
