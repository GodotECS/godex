#pragma once

#include "../../databags/databag.h"

class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btConstraintSolver;
class btDiscreteDynamicsWorld;
class btSoftBodyWorldInfo;
class btGhostPairCallback;
class GodotFilterCallback;
class BtSpaces;

class BtSpace {
	friend class BtSpaces;

	btBroadphaseInterface *broadphase = nullptr;
	btDefaultCollisionConfiguration *collision_configuration = nullptr;
	btCollisionDispatcher *dispatcher = nullptr;
	btConstraintSolver *solver = nullptr;
	btDiscreteDynamicsWorld *dynamics_world = nullptr;
	btGhostPairCallback *ghost_pair_callback = nullptr;
	GodotFilterCallback *godot_filter_callback = nullptr;
	btSoftBodyWorldInfo *soft_body_world_info = nullptr;

public:
	btBroadphaseInterface *get_broadphase() { return broadphase; }
	const btBroadphaseInterface *get_broadphase() const { return broadphase; }

	btDefaultCollisionConfiguration *get_collisionConfiguration() { return collision_configuration; }
	const btDefaultCollisionConfiguration *get_collisionConfiguration() const { return collision_configuration; }

	btCollisionDispatcher *get_dispatcher() { return dispatcher; }
	const btCollisionDispatcher *get_dispatcher() const { return dispatcher; }

	btConstraintSolver *get_solver() { return solver; }
	const btConstraintSolver *get_solver() const { return solver; }

	btDiscreteDynamicsWorld *get_dynamicsWorld() { return dynamics_world; }
	const btDiscreteDynamicsWorld *get_dynamicsWorld() const { return dynamics_world; }

	btGhostPairCallback *get_ghostPairCallback() { return ghost_pair_callback; }
	const btGhostPairCallback *get_ghostPairCallback() const { return ghost_pair_callback; }

	GodotFilterCallback *get_godotFilterCallback() { return godot_filter_callback; }
	const GodotFilterCallback *get_godotFilterCallback() const { return godot_filter_callback; }
};

/// The `BtSpaces` is a databag that contains all the physics worlds
/// (called spaces).
/// You can have at max 4 spaces at the same time, but note that have more than
/// one is not a common use case.
///
/// You can specify to which `Space` an entity has to stay by using the
/// optional `BtSpaceMarker`. When the `BtSpaceMarker` is not used the `Entity`
/// is put to the main default world (ID 0).
class BtSpaces : public godex::Databag {
	DATABAG(BtSpaces)

public:
	enum SpaceID {
		SPACE_0 = 0,
		SPACE_1 = 1,
		SPACE_2 = 2,
		SPACE_3 = 3,
		SPACE_MAX = 4,
	};

private:
	BtSpace spaces[SPACE_MAX];

public:
	BtSpaces();
	~BtSpaces();

	/// Returns `true` if the space is initialized.
	bool is_space_initialized(SpaceID p_id) const;

	/// Initialize the space pointeed by this ID. If this space is already
	/// initialize does nothing.
	void init_space(SpaceID p_id, bool p_soft_world);

	/// Free the space pointed by this ID, or does nothing if the space is not
	/// initialized.
	void free_space(SpaceID p_id);

	/// Returns the Space of this ID mutable.
	BtSpace *get_space(SpaceID p_id);

	/// Returns the Space of this ID, not mutable.
	const BtSpace *get_space(SpaceID p_space_id) const;
};
