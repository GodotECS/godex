
#include "components_rigid_body.h"

#include "databag_world.h"
#include <btBulletCollisionCommon.h>

void GodexBtMotionState::getWorldTransform(btTransform &r_world_trans) const {
	r_world_trans = transf;
}

void GodexBtMotionState::setWorldTransform(const btTransform &worldTrans) {
	transf = worldTrans;
	ERR_FAIL_COND_MSG(world == nullptr, "Body moved while no world is set, this is a bug!");
	world->moved_bodies.insert(entity);
}

void BtWorldMarker::_bind_methods() {
	ECS_BIND_PROPERTY(BtWorldMarker, PropertyInfo(Variant::INT, "world_index", PROPERTY_HINT_ENUM, "World 0 (main),World 1,World 2,World 3,None"), world_index);
}

void BtRigidBody::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtRigidBody, PropertyInfo(Variant::INT, "body_mode", PROPERTY_HINT_ENUM, "Dynamic,Character,Kinematic,Static"), script_set_body_mode, get_body_mode);
	ECS_BIND_PROPERTY_FUNC(BtRigidBody, PropertyInfo(Variant::FLOAT, "mass", PROPERTY_HINT_RANGE, "0,1000,0.01,1"), set_mass, get_mass);
	ECS_BIND_PROPERTY_FUNC(BtRigidBody, PropertyInfo(Variant::INT, "layer", PROPERTY_HINT_LAYERS_3D_PHYSICS), set_layer, get_layer);
	ECS_BIND_PROPERTY_FUNC(BtRigidBody, PropertyInfo(Variant::INT, "mask", PROPERTY_HINT_LAYERS_3D_PHYSICS), set_mask, get_mask);
}

void BtRigidBody::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 500 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 500;
}

btRigidBody *BtRigidBody::get_body() {
	return &body;
}

const btRigidBody *BtRigidBody::get_body() const {
	return &body;
}

GodexBtMotionState *BtRigidBody::get_motion_state() {
	return &motion_state;
}

const GodexBtMotionState *BtRigidBody::get_motion_state() const {
	return &motion_state;
}

void BtRigidBody::script_set_body_mode(uint32_t p_mode) {
	set_body_mode(static_cast<RigidMode>(p_mode));
}

void BtRigidBody::set_body_mode(RigidMode p_mode) {
	int cleared_current_flags = body.getCollisionFlags();
	cleared_current_flags &= ~(btCollisionObject::CF_KINEMATIC_OBJECT |
							   btCollisionObject::CF_STATIC_OBJECT |
							   btCollisionObject::CF_CHARACTER_OBJECT);

	if (p_mode == RIGID_MODE_DYNAMIC) {
		body.setCollisionFlags(cleared_current_flags); // Just set the flags without Kin and Static
		body.forceActivationState(ACTIVE_TAG);
		body.setLinearFactor(btVector3(1, 1, 1));
		body.setAngularFactor(btVector3(1, 1, 1));

	} else if (p_mode == RIGID_MODE_CHARACTER) {
		body.setCollisionFlags(cleared_current_flags |
							   btCollisionObject::CF_CHARACTER_OBJECT);
		body.forceActivationState(ACTIVE_TAG);
		body.setLinearFactor(btVector3(1, 1, 1));
		body.setAngularFactor(btVector3(0, 0, 0));

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

	reload_flags |= RELOAD_FLAGS_MASS;
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
	reload_flags |= RELOAD_FLAGS_MASS;
}

real_t BtRigidBody::get_mass() const {
	return mass;
}

bool BtRigidBody::need_mass_reload() const {
	return reload_flags & RELOAD_FLAGS_MASS;
}

void BtRigidBody::reload_mass(const btCollisionShape *p_shape) {
	// Change the mass depending on the Body Mode.
	const RigidMode mode = get_body_mode();
	real_t n_mass = mass;
	if (mode == RIGID_MODE_DYNAMIC || mode == RIGID_MODE_CHARACTER) {
		// If Dynamic the mass can't be less than 0.001
		n_mass = MAX(0.001, n_mass);
	} else {
		n_mass = 0;
	}

	btVector3 local_inertia(0, 0, 0);
	if (p_shape) {
		p_shape->calculateLocalInertia(n_mass, local_inertia);
	}
	body.setMassProps(n_mass, local_inertia);

	reload_flags &= (~RELOAD_FLAGS_MASS);
}

void BtRigidBody::set_layer(uint32_t p_layer) {
	layer = p_layer;
	reload_flags |= RELOAD_FLAGS_BODY;
}

uint32_t BtRigidBody::get_layer() const {
	return layer;
}

void BtRigidBody::set_mask(uint32_t p_mask) {
	mask = p_mask;
	reload_flags |= RELOAD_FLAGS_BODY;
}

uint32_t BtRigidBody::get_mask() const {
	return mask;
}

bool BtRigidBody::need_body_reload() const {
	return reload_flags & RELOAD_FLAGS_BODY;
}

void BtRigidBody::reload_body(BtWorldIndex p_index) {
	__current_world = p_index;
	reload_flags &= (~RELOAD_FLAGS_BODY);
}

void BtRigidBody::set_shape(btCollisionShape *p_shape) {
	body.setCollisionShape(p_shape);
	reload_flags |= RELOAD_FLAGS_MASS;
}

btCollisionShape *BtRigidBody::get_shape() {
	return body.getCollisionShape();
}

const btCollisionShape *BtRigidBody::get_shape() const {
	return body.getCollisionShape();
}
