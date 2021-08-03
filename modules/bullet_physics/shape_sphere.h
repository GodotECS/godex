#pragma once

#include "../../databags/databag.h"
#include "core/templates/paged_allocator.h"
#include "shape_base.h"
#include <BulletCollision/CollisionShapes/btSphereShape.h>

struct BtSphere : public BtRigidShape {
	COMPONENT_CUSTOM_CONSTRUCTOR(BtSphere, SharedSteadyStorage)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	real_t radius = 1.0;

	BtSphere() :
			BtRigidShape(TYPE_SPHERE) {}

	void set_radius(real_t p_radius);
	real_t get_radius() const;

	ShapeInfo *add_shape(btSphereShape *p_shape, const Vector3 &p_scale);
};

struct BtShapeStorageSphere : public godex::Databag {
	DATABAG(BtShapeStorageSphere);

	PagedAllocator<btSphereShape, false> allocator;

	btCollisionShape *construct_shape(BtSphere *p_shape_owner, const Vector3 &p_scale);
};
