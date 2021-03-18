#include "databag_space.h"

#include "core/config/project_settings.h"
#include "modules/bullet/godot_collision_configuration.h"
#include "modules/bullet/godot_collision_dispatcher.h"
#include "modules/bullet/godot_result_callbacks.h"
#include <BulletCollision/BroadphaseCollision/btBroadphaseProxy.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.h>
#include <BulletCollision/NarrowPhaseCollision/btGjkPairDetector.h>
#include <BulletCollision/NarrowPhaseCollision/btPointCollector.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <btBulletDynamicsCommon.h>

btScalar calculate_godot_combined_restitution(const btCollisionObject *body0, const btCollisionObject *body1) {
	return CLAMP(body0->getRestitution() + body1->getRestitution(), 0, 1);
}

btScalar calculate_godot_combined_friction(const btCollisionObject *body0, const btCollisionObject *body1) {
	return ABS(MIN(body0->getFriction(), body1->getFriction()));
}

void on_post_tick_callback(btDynamicsWorld *p_dynamics_world, btScalar p_delta) {
	//BtWorld *space = static_cast<BtWorld *>(p_dynamics_world->getWorldUserInfo());
}

BtPhysicsSpaces::BtPhysicsSpaces() {
	// Always init the space 0, which is the default one.
	init_space(BT_SPACE_0, GLOBAL_DEF("physics/3d/active_soft_world", true));
	CRASH_COND_MSG(is_space_initialized(BT_SPACE_0) == false, "At this point the space 0 is expected to be initialized.");
}

BtPhysicsSpaces::~BtPhysicsSpaces() {
	// Free all the spaces.
	for (uint32_t i = 0; i < BT_SPACE_MAX; i += 1) {
		free_space(static_cast<BtSpaceIndex>(i));
	}
}

bool BtPhysicsSpaces::is_space_initialized(BtSpaceIndex p_id) const {
	return spaces[p_id].broadphase != nullptr;
}

void BtPhysicsSpaces::init_space(BtSpaceIndex p_id, bool p_soft_world) {
	ERR_FAIL_COND_MSG(is_space_initialized(p_id), "This space " + itos(p_id) + " is already initialized");

	void *world_mem;
	if (p_soft_world) {
		world_mem = malloc(sizeof(btSoftRigidDynamicsWorld));
		spaces[p_id].collision_configuration = memnew(GodotSoftCollisionConfiguration(
				static_cast<btDiscreteDynamicsWorld *>(world_mem)));
	} else {
		world_mem = malloc(sizeof(btDiscreteDynamicsWorld));
		spaces[p_id].collision_configuration = memnew(GodotCollisionConfiguration(
				static_cast<btDiscreteDynamicsWorld *>(world_mem)));
	}

	spaces[p_id].dispatcher = memnew(GodotCollisionDispatcher(spaces[p_id].collision_configuration));
	spaces[p_id].broadphase = memnew(btDbvtBroadphase);
	spaces[p_id].solver = new btSequentialImpulseConstraintSolver;

	if (p_soft_world) {
		spaces[p_id].dynamics_world =
				new (world_mem) btSoftRigidDynamicsWorld(
						spaces[p_id].dispatcher,
						spaces[p_id].broadphase,
						spaces[p_id].solver,
						spaces[p_id].collision_configuration);
		spaces[p_id].soft_body_world_info = memnew(btSoftBodyWorldInfo);
	} else {
		spaces[p_id].dynamics_world =
				new (world_mem) btDiscreteDynamicsWorld(
						spaces[p_id].dispatcher,
						spaces[p_id].broadphase,
						spaces[p_id].solver,
						spaces[p_id].collision_configuration);
	}

	// Set the space as User Info pointer, so I can access this space within
	// bullet callbacks.
	// Note, the `spaces` memory location doesn't change, so it's fine pass the
	// pointer.
	spaces[p_id].dynamics_world->setWorldUserInfo(spaces + p_id);

	// Set global callbacks.
	gCalculateCombinedRestitutionCallback = &calculate_godot_combined_restitution;
	gCalculateCombinedFrictionCallback = &calculate_godot_combined_friction;
	gContactAddedCallback = &godotContactAddedCallback;

	// Setup the world callbacks.
	spaces[p_id].ghost_pair_callback = memnew(btGhostPairCallback);
	spaces[p_id].godot_filter_callback = memnew(GodotFilterCallback);

	spaces[p_id].dynamics_world->setInternalTickCallback(on_post_tick_callback, this, false);
	spaces[p_id].dynamics_world->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(
			spaces[p_id].ghost_pair_callback);
	spaces[p_id].dynamics_world->getPairCache()->setOverlapFilterCallback(
			spaces[p_id].godot_filter_callback);
}

void BtPhysicsSpaces::free_space(BtSpaceIndex p_id) {
	if (is_space_initialized(p_id)) {
		// Nothing to do.
		return;
	}

	memdelete(spaces[p_id].broadphase);
	spaces[p_id].broadphase = nullptr;

	memdelete(spaces[p_id].collision_configuration);
	spaces[p_id].collision_configuration = nullptr;

	memdelete(spaces[p_id].dispatcher);
	spaces[p_id].dispatcher = nullptr;

	delete spaces[p_id].solver;
	spaces[p_id].solver = nullptr;

	memdelete(spaces[p_id].dynamics_world);
	spaces[p_id].dynamics_world = nullptr;

	memdelete(spaces[p_id].ghost_pair_callback);
	spaces[p_id].ghost_pair_callback = nullptr;

	memdelete(spaces[p_id].godot_filter_callback);
	spaces[p_id].godot_filter_callback = nullptr;
}

BtSpace *BtPhysicsSpaces::get_space(BtSpaceIndex p_index) {
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND_V_MSG(p_index >= BT_SPACE_MAX, spaces, "This index is out of bounds.");
#endif
	return spaces + p_index;
}

const BtSpace *BtPhysicsSpaces::get_space(BtSpaceIndex p_index) const {
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND_V_MSG(p_index >= BT_SPACE_MAX, spaces, "This index is out of bounds.");
#endif
	return spaces + p_index;
}
