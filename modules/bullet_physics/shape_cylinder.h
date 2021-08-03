#pragma once

#include "../../databags/databag.h"
#include "core/templates/paged_allocator.h"
#include "shape_base.h"
#include <BulletCollision/CollisionShapes/btCylinderShape.h>

struct BtCylinder : public BtRigidShape {
	COMPONENT_CUSTOM_CONSTRUCTOR(BtCylinder, SharedSteadyStorage)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	real_t radius = 1.0;
	real_t height = 1.0;

	BtCylinder() :
			BtRigidShape(TYPE_CONE) {}

	void set_radius(real_t p_radius);
	real_t get_radius() const;

	void set_height(real_t p_height);
	real_t get_height() const;

	ShapeInfo *add_shape(btCylinderShape *p_shape, const Vector3 &p_scale);

private:
	void update_shapes();
};

struct BtShapeStorageCylinder : public godex::Databag {
	DATABAG(BtShapeStorageCylinder);

	PagedAllocator<btCylinderShape, false> allocator;

	btCollisionShape *construct_shape(BtCylinder *p_shape_owner, const Vector3 &p_scale);
};
