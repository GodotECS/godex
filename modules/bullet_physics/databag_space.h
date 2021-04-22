#pragma once

#include "../../databags/databag.h"
#include "../../storage/entity_list.h"
#include "bt_def_type.h"
#include <BulletCollision/CollisionShapes/btEmptyShape.h>

class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btConstraintSolver;
class btDiscreteDynamicsWorld;
struct btSoftBodyWorldInfo;
class btGhostPairCallback;
struct GodexBtFilterCallback;
class BtPhysicsSpaces;

class BtSpace {
	friend class BtPhysicsSpaces;

	btBroadphaseInterface *broadphase = nullptr;
	btDefaultCollisionConfiguration *collision_configuration = nullptr;
	btCollisionDispatcher *dispatcher = nullptr;
	btConstraintSolver *solver = nullptr;
	btDiscreteDynamicsWorld *dynamics_world = nullptr;
	btGhostPairCallback *ghost_pair_callback = nullptr;
	GodexBtFilterCallback *godot_filter_callback = nullptr;
	btSoftBodyWorldInfo *soft_body_world_info = nullptr;

public:
	EntityList moved_bodies;

	btBroadphaseInterface *get_broadphase() { return broadphase; }
	const btBroadphaseInterface *get_broadphase() const { return broadphase; }

	btDefaultCollisionConfiguration *get_collision_configuration() { return collision_configuration; }
	const btDefaultCollisionConfiguration *get_collision_configuration() const { return collision_configuration; }

	btCollisionDispatcher *get_dispatcher() { return dispatcher; }
	const btCollisionDispatcher *get_dispatcher() const { return dispatcher; }

	btConstraintSolver *get_solver() { return solver; }
	const btConstraintSolver *get_solver() const { return solver; }

	btDiscreteDynamicsWorld *get_dynamics_world() { return dynamics_world; }
	const btDiscreteDynamicsWorld *get_dynamics_world() const { return dynamics_world; }

	btGhostPairCallback *get_ghost_pair_callback() { return ghost_pair_callback; }
	const btGhostPairCallback *get_ghost_pair_callback() const { return ghost_pair_callback; }

	GodexBtFilterCallback *get_godot_filter_callback() { return godot_filter_callback; }
	const GodexBtFilterCallback *get_godot_filter_callback() const { return godot_filter_callback; }
};

/// The `BtPhysicsSpaces` is a databag that contains all the physics worlds
/// (called spaces).
/// You can have at max 4 spaces at the same time, but note that have more than
/// one is not a common use case.
///
/// You can specify to which `Space` an entity has to stay by using the
/// optional `BtSpaceMarker`. When the `BtSpaceMarker` is not used the `Entity`
/// is put to the main default world (ID 0).
class BtPhysicsSpaces : public godex::Databag {
	DATABAG(BtPhysicsSpaces)

public:
	btEmptyShape empty_shape;

private:
	BtSpace spaces[BT_SPACE_MAX];

public:
	BtPhysicsSpaces();
	~BtPhysicsSpaces();

	/// Returns `true` if the space is initialized.
	bool is_space_initialized(BtSpaceIndex p_id) const;

	/// Initialize the space pointeed by this ID. If this space is already
	/// initialize does nothing.
	void init_space(BtSpaceIndex p_id, bool p_soft_world);

	/// Free the space pointed by this ID, or does nothing if the space is not
	/// initialized.
	void free_space(BtSpaceIndex p_id);

	/// Returns the Space of this ID mutable.
	BtSpace *get_space(BtSpaceIndex p_id);

	/// Returns the Space of this ID, not mutable.
	const BtSpace *get_space(BtSpaceIndex p_space_id) const;
};

/// This databags is used to hold the bullet physics cache.
class BtCache : public godex::Databag {
	DATABAG(BtCache)

	/// Counter used by the overlap check to detect the IN and OUT bodies.
	uint32_t area_check_frame_counter = 0;
};
