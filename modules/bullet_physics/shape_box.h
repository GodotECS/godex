#pragma once

#include "../../databags/databag.h"
#include "core/templates/paged_allocator.h"
#include "shape_base.h"
#include <BulletCollision/CollisionShapes/btBoxShape.h>

struct BtBox : public BtRigidShape {
	COMPONENT_CUSTOM_CONSTRUCTOR(BtBox, SharedSteadyStorage)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	real_t margin = 0.004;
	Vector3 half_extends = Vector3(1, 1, 1);

	BtBox() :
			BtRigidShape(TYPE_BOX) {}

	void set_half_extents(const Vector3 &p_half_extends);
	Vector3 get_half_extents() const;

	void set_margin(real_t p_margin);
	real_t get_margin() const;

	ShapeInfo *add_shape(btBoxShape *p_shape, const Vector3 &p_scale);
};

struct BtShapeStorageBox : public godex::Databag {
	DATABAG(BtShapeStorageBox);

	PagedAllocator<btBoxShape, false> allocator;

	btCollisionShape *construct_shape(BtBox *p_shape_owner, const Vector3 &p_scale);
};
