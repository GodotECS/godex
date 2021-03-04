
#include "components.h"

#include <btBulletCollisionCommon.h>

void BtSpaceMarker::_bind_methods() {
	ECS_BIND_PROPERTY(BtSpaceMarker, PropertyInfo(Variant::INT, "space_id", PROPERTY_HINT_ENUM, "Space 0 (main),Space 1,Space 2,Space 3"), space_id);
}

void BtRigidBody::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtRigidBody, PropertyInfo(Variant::INT, "body_mode", PROPERTY_HINT_ENUM, "Dynamic,Character,Kinematic,Static"), script_set_body_mode, get_body_mode);
}

void BtRigidBody::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 500 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 500;
}

void BtRigidBody::script_set_body_mode(uint32_t p_mode) {
	set_body_mode(static_cast<RigidMode>(p_mode));
}

void BtRigidBody::set_body_mode(RigidMode p_mode) {
	int cleared_current_flags = body.getCollisionFlags();
	cleared_current_flags &= ~(btCollisionObject::CF_KINEMATIC_OBJECT |
							   btCollisionObject::CF_STATIC_OBJECT |
							   btCollisionObject::CF_CHARACTER_OBJECT);

	btVector3 local_inertia(0, 0, 0);

	if (p_mode == RIGID_MODE_DYNAMIC) {
		body.setCollisionFlags(cleared_current_flags); // Just set the flags without Kin and Static
		mass = body.getMass() == 0.0 ? 1.0 : body.getMass();
		if (main_shape) {
			main_shape->calculateLocalInertia(mass, local_inertia);
		}
		body.forceActivationState(ACTIVE_TAG);

	} else if (p_mode == RIGID_MODE_CHARACTER) {
		body.setCollisionFlags(cleared_current_flags |
							   btCollisionObject::CF_CHARACTER_OBJECT);
		mass = body.getMass() == 0.0 ? 1.0 : body.getMass();
		if (main_shape) {
			main_shape->calculateLocalInertia(mass, local_inertia);
		}
		body.forceActivationState(ACTIVE_TAG);

	} else if (p_mode == RIGID_MODE_KINEMATIC) {
		body.setCollisionFlags(cleared_current_flags |
							   btCollisionObject::CF_KINEMATIC_OBJECT);
		body.forceActivationState(DISABLE_SIMULATION);

	} else {
		// Mode is STATIC
		body.setCollisionFlags(cleared_current_flags |
							   btCollisionObject::CF_STATIC_OBJECT);
		body.forceActivationState(DISABLE_SIMULATION);
	}

	body.setMassProps(mass, local_inertia);
}

BtRigidBody::RigidMode BtRigidBody::get_body_mode() const {
	const int current_flags = body.getCollisionFlags();
	if (current_flags & btCollisionObject::CF_CHARACTER_OBJECT) {
		return RIGID_MODE_CHARACTER;
	} else if (current_flags & btCollisionObject::CF_KINEMATIC_OBJECT) {
		return RIGID_MODE_KINEMATIC;
	} else if (current_flags & btCollisionObject::CF_STATIC_OBJECT) {
		return RIGID_MODE_STATIC;
	} else {
		return RIGID_MODE_DYNAMIC;
	}
}

void BtRigidBody::set_mass(real_t p_mass) {
	// In Bullet Physics, the mass depends on the BodyMode: When the body is
	// dynamic or character the mass must always be more that 0.0
	// The mass is always stored, so we don't lose the mass the User set,
	// so we can change mass and body mode in any order.
	mass = p_mass;

	// Change the mass depending on the Body Mode.
	const RigidMode mode = get_body_mode();
	if (mode == RIGID_MODE_DYNAMIC || mode == RIGID_MODE_CHARACTER) {
		// If Dynamic the mass can't be less than 0.001
		p_mass = MAX(0.001, p_mass);
	} else {
		p_mass = 0;
	}

	btVector3 local_inertia(0, 0, 0);
	if (main_shape) {
		main_shape->calculateLocalInertia(p_mass, local_inertia);
	}
	body.setMassProps(p_mass, local_inertia);
}

real_t BtRigidBody::get_mass() const {
	return mass;
}
