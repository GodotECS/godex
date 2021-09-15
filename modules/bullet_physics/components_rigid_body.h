#pragma once

#include "../../components/component.h"
#include "../../storage/dense_vector_storage.h"
#include "../../storage/shared_steady_storage.h"
#include "../../storage/steady_storage.h"
#include "bt_def_type.h"
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <LinearMath/btMotionState.h>

class btCollisionShape;
class InterpolatedTransformComponent;

/// This is an optional component, that allow to specify a specific physics
/// `Space` where the `Entity` is put.
/// When this component is not set, the `Entity` is put to the default main
/// space.
///
/// Note, you have at max 4 spaces, and it's unlikely that you need more than 1.
struct BtSpaceMarker {
	COMPONENT(BtSpaceMarker, DenseVectorStorage)

	static void _bind_methods();

	uint32_t space_index = BT_SPACE_0;
};

/// This class is an utility Bullet physics uses to notify the RigidBody
/// transform change.
class GodexBtMotionState : public btMotionState {
	friend struct BtRigidBody;

public:
	EntityID entity;
	class BtSpace *space = nullptr;
	btTransform transf = btTransform(
			btMatrix3x3(1., 0., 0., 0., 1., 0., 0., 0., 1.),
			btVector3(0., 0., 0.));

	/// NEVER CALL THIS FUNCTION.
	///
	/// When the body is Kinematic, bullet calls this function to
	/// set the body position, so it's possible to interpolate it.
	virtual void getWorldTransform(btTransform &r_world_trans) const override;

	/// Bullet physics call this function on active bodies to update the
	/// position.
	/// The given Transform is already interpolated by bullet, is substepping
	/// is active.
	virtual void setWorldTransform(const btTransform &worldTrans) override;

	void notify_transform_changed();
};

/// This Component represent a Bullet Physics RigidBody.
/// The RigidBody can be STATIC, DYNAMIC, KINEMATIC.
struct BtRigidBody {
	friend class GodexBtMotionState;

	COMPONENT_CUSTOM_CONSTRUCTOR(BtRigidBody, SteadyStorage)

	enum RigidMode {
		RIGID_MODE_DYNAMIC,
		RIGID_MODE_CHARACTER,
		RIGID_MODE_KINEMATIC,
		RIGID_MODE_STATIC
	};

	enum ReloadFlags {
		/// Reload the mass.
		RELOAD_FLAGS_MASS = 1 << 0,
		/// Remove and insert the body into the world again.
		RELOAD_FLAGS_BODY = 1 << 1,
	};

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

public:
	/// The current space this body is. Do not modify this.
	BtSpaceIndex __current_space = BT_SPACE_NONE;
	/// The current mode this body has. Do not modify this.
	RigidMode __current_mode = RIGID_MODE_STATIC;
	Vector3 body_scale = Vector3(1, 1, 1);

private:
	real_t mass = 1.0;
	btRigidBody body = btRigidBody(mass, nullptr, nullptr, btVector3(0.0, 0.0, 0.0));
	GodexBtMotionState motion_state;

	uint32_t layer = 1;
	uint32_t mask = 1;

	uint32_t reload_flags = 0;

public:
	BtRigidBody();

	btRigidBody *get_body();
	const btRigidBody *get_body() const;

	GodexBtMotionState *get_motion_state();
	const GodexBtMotionState *get_motion_state() const;

	const btTransform &get_transform() const;
	void set_transform(const btTransform &p_transform, bool p_notify_changed);

	/// Teleports the body to the given transform location.
	/// It's possible to optionally pass the InterpolatedTransformComponent to
	/// update it right away, and avoid glitches.
	/// Notice: It's adviced to pass that component.
	void teleport(const btTransform &p_transform, InterpolatedTransformComponent *p_interpolation_component);

	void script_set_body_mode(uint32_t p_mode);
	void set_body_mode(RigidMode p_mode);
	RigidMode get_body_mode() const;

	void set_mass(real_t p_mass);
	real_t get_mass() const;

	bool need_mass_reload() const;
	/// Reload the mass.
	void reload_mass();

	void set_layer(uint32_t p_layer);
	uint32_t get_layer() const;

	void set_mask(uint32_t p_mask);
	uint32_t get_mask() const;

	void set_linear_factor(const Vector3 &p_factor);
	Vector3 get_linear_factor() const;

	void set_angular_factor(const Vector3 &p_factor);
	Vector3 get_angular_factor() const;

	void set_friction(real_t p_friction);
	real_t get_friction() const;

	void set_bounciness(real_t p_bounciness);
	real_t get_bounciness() const;

	bool need_body_reload() const;
	void reload_body(BtSpaceIndex p_index);

	void set_shape(btCollisionShape *p_shape);
	btCollisionShape *get_shape();
	const btCollisionShape *get_shape() const;
};
