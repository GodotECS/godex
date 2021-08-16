#pragma once

#include "../../databags/databag.h"
#include "core/templates/paged_allocator.h"
#include "shape_base.h"
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>

// This class was created, to implement the API to change the shape size after
// its creation.
struct GodexBtCapsuleShape : public btCapsuleShape {
	GodexBtCapsuleShape() :
			btCapsuleShape(1.0, 1.0) {}

	void set_radius(real_t p_radius);
	void set_height(real_t p_height);
};

struct BtCapsule : public BtRigidShape {
	COMPONENT_CUSTOM_CONSTRUCTOR(BtCapsule, SharedSteadyStorage)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	real_t radius = 1.0;
	real_t height = 1.0;

	BtCapsule() :
			BtRigidShape(TYPE_CAPSULE) {}

	void set_radius(real_t p_radius);
	real_t get_radius() const;

	void set_height(real_t p_height);
	real_t get_height() const;

	ShapeInfo *add_shape(GodexBtCapsuleShape *p_shape, const Vector3 &p_scale);

private:
	void update_shapes();
};

struct BtShapeStorageCapsule : public godex::Databag {
	DATABAG(BtShapeStorageCapsule);

	PagedAllocator<GodexBtCapsuleShape, false> allocator;

	btCollisionShape *construct_shape(BtCapsule *p_capsule, const Vector3 &p_scale);
};
