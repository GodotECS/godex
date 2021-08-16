#pragma once

#include "../../databags/databag.h"
#include "core/templates/paged_allocator.h"
#include "shape_base.h"
#include <BulletCollision/CollisionShapes/btConvexPointCloudShape.h>

struct BtConvex : public BtRigidShape {
	COMPONENT_CUSTOM_CONSTRUCTOR(BtConvex, SharedSteadyStorage)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	btVector3 *points = nullptr;
	uint32_t point_count = 0;

	BtConvex() :
			BtRigidShape(TYPE_CONVEX) {}

	// Copy constructor is needed because I'm dealing with pointers here.
	BtConvex(const BtConvex &p_other);
	BtConvex &operator=(const BtConvex &p_other);
	~BtConvex();

	void set_points(const Vector<Vector3> &p_points);
	Vector<Vector3> get_points() const;

	ShapeInfo *add_shape(btConvexPointCloudShape *p_shape, const Vector3 &p_scale);

private:
	void update_shapes();
};

struct BtShapeStorageConvex : public godex::Databag {
	DATABAG(BtShapeStorageConvex);

	PagedAllocator<btConvexPointCloudShape, false> allocator;

	btCollisionShape *construct_shape(BtConvex *p_shape_owner, const Vector3 &p_scale);
};
