#pragma once

#include "../../databags/databag.h"
#include "bt_def_type.h"

class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btConstraintSolver;
class btDiscreteDynamicsWorld;
struct btSoftBodyWorldInfo;
class btGhostPairCallback;
struct GodotFilterCallback;
class BtWorlds;

class BtWorld {
	friend class BtWorlds;

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

	GodotFilterCallback *get_godot_filter_callback() { return godot_filter_callback; }
	const GodotFilterCallback *get_godot_filter_callback() const { return godot_filter_callback; }
};

/// The `BtWorlds` is a databag that contains all the physics worlds
/// (called spaces).
/// You can have at max 4 spaces at the same time, but note that have more than
/// one is not a common use case.
///
/// You can specify to which `Space` an entity has to stay by using the
/// optional `BtWorldMarker`. When the `BtWorldMarker` is not used the `Entity`
/// is put to the main default world (ID 0).
class BtWorlds : public godex::Databag {
	DATABAG(BtWorlds)

private:
	BtWorld spaces[BT_WOLRD_MAX];

public:
	BtWorlds();
	~BtWorlds();

	/// Returns `true` if the space is initialized.
	bool is_space_initialized(BtWorldIndex p_id) const;

	/// Initialize the space pointeed by this ID. If this space is already
	/// initialize does nothing.
	void init_space(BtWorldIndex p_id, bool p_soft_world);

	/// Free the space pointed by this ID, or does nothing if the space is not
	/// initialized.
	void free_space(BtWorldIndex p_id);

	/// Returns the Space of this ID mutable.
	BtWorld *get_space(BtWorldIndex p_id);

	/// Returns the Space of this ID, not mutable.
	const BtWorld *get_space(BtWorldIndex p_space_id) const;
};
