#pragma once

#include "../../components/component.h"
#include "../../storage/shared_steady_storage.h"
#include "../../storage/steady_storage.h"
#include <btBulletCollisionCommon.h>

struct BtRigidShape {
	enum ShapeType {
		TYPE_BOX,
		TYPE_SPHERE
	};

protected:
	/// This is used to know the type of the shape, so we know how to extract
	/// the shape common info, like the `btCollisionShape`.
	ShapeType type;

	/// List of bodies where this shape is assigned.
	/// TODO Do I really need this, can I take this infor from the storage instead??
	LocalVector<EntityID> bodies;

public:
	BtRigidShape(ShapeType p_type) :
			type(p_type) {}

	btCollisionShape *get_shape();
	const btCollisionShape *get_shape() const;

	void add_body(EntityID p_entity) {
		bodies.push_back(p_entity);
	}
};

struct BtShapeBox : public BtRigidShape {
	COMPONENT_CUSTOM_CONSTRUCTOR(BtShapeBox, SteadyStorage) // TODO Please used the SharedComponent instead.

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	btBoxShape box = btBoxShape(btVector3(1.0, 1.0, 1.0));

	BtShapeBox() :
			BtRigidShape(TYPE_BOX) {}

	void set_half_extents(const Vector3 &p_half_extends);
	Vector3 get_half_extents() const;

	void set_margin(real_t p_margin);
	real_t get_margin() const;
};

struct BtShapeSphere : public BtRigidShape {
	COMPONENT_CUSTOM_CONSTRUCTOR(BtShapeSphere, SteadyStorage) // TODO Please use SharedComponent

	btSphereShape sphere = btSphereShape(1.0);

	BtShapeSphere() :
			BtRigidShape(TYPE_SPHERE) {}

	// TODO finalize shape.
};
