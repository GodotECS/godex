#pragma once

#include "../../godot/editor_plugins/components_gizmo_3d.h"

class BtBoxGizmo : public ComponentGizmo {
	StringName box_component_name = "BtBox";
	StringName half_extents_name = "half_extents";

public:
	virtual void init() override;
	virtual void redraw(EditorNode3DGizmo *p_gizmo) override;
	virtual int get_handle_count(const EditorNode3DGizmo *p_gizmo) const override;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual Variant get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual void set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) override;
	virtual void commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) override;
};

class BtSphereGizmo : public ComponentGizmo {
	StringName sphere_component_name = "BtSphere";
	StringName radius_name = "radius";

public:
	virtual void init() override;
	virtual void redraw(EditorNode3DGizmo *p_gizmo) override;
	virtual int get_handle_count(const EditorNode3DGizmo *p_gizmo) const override;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual Variant get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual void set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) override;
	virtual void commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) override;
};

class BtCapsuleGizmo : public ComponentGizmo {
	StringName capsule_component_name = "BtCapsule";
	StringName radius_name = "radius";
	StringName height_name = "height";

public:
	virtual void init() override;
	virtual void redraw(EditorNode3DGizmo *p_gizmo) override;
	virtual int get_handle_count(const EditorNode3DGizmo *p_gizmo) const override;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual Variant get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual void set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) override;
	virtual void commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) override;
};

class BtConeGizmo : public ComponentGizmo {
	StringName cone_component_name = "BtCone";
	StringName radius_name = "radius";
	StringName height_name = "height";

public:
	virtual void init() override;
	virtual void redraw(EditorNode3DGizmo *p_gizmo) override;
	virtual int get_handle_count(const EditorNode3DGizmo *p_gizmo) const override;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual Variant get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual void set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) override;
	virtual void commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) override;
};

class BtCylinderGizmo : public ComponentGizmo {
	StringName cylinder_component_name = "BtCylinder";
	StringName radius_name = "radius";
	StringName height_name = "height";

public:
	virtual void init() override;
	virtual void redraw(EditorNode3DGizmo *p_gizmo) override;
	virtual int get_handle_count(const EditorNode3DGizmo *p_gizmo) const override;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual Variant get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual void set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) override;
	virtual void commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) override;
};

class BtConvexGizmo : public ComponentGizmo {
	StringName convex_component_name = "BtConvex";
	StringName points_name = "points";

public:
	virtual void init() override;
	virtual void redraw(EditorNode3DGizmo *p_gizmo) override;
	virtual int get_handle_count(const EditorNode3DGizmo *p_gizmo) const override;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual Variant get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual void set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) override;
	virtual void commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) override;
};

class BtTrimeshGizmo : public ComponentGizmo {
	StringName trimesh_component_name = "BtTrimesh";
	StringName faces_name = "faces";

public:
	virtual void init() override;
	virtual void redraw(EditorNode3DGizmo *p_gizmo) override;
	virtual int get_handle_count(const EditorNode3DGizmo *p_gizmo) const override;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual Variant get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual void set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) override;
	virtual void commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) override;
};

class BtPawnGizmo : public ComponentGizmo {
	StringName pawn_component_name = "BtPawn";

	StringName stance0_radius_name = "stance0_pawn_radius";
	StringName stance0_height_name = "stance0_pawn_height";

	StringName stance1_radius_name = "stance1_pawn_radius";
	StringName stance1_height_name = "stance1_pawn_height";

public:
	virtual void init() override;

	virtual void redraw(EditorNode3DGizmo *p_gizmo) override;
	void redraw_capsule(EditorNode3DGizmo *p_gizmo, const Ref<Material> material, real_t p_height, real_t p_radius, const Vector3 &p_offset);

	virtual int get_handle_count(const EditorNode3DGizmo *p_gizmo) const override;
	virtual String get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual Variant get_handle_value(const EditorNode3DGizmo *p_gizmo, int p_idx) const override;
	virtual void set_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) override;
	virtual void commit_handle(const EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel = false) override;
};
