#include "bt_systems.h"

#include <BulletCollision/BroadphaseCollision/btBroadphaseProxy.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.h>
#include <BulletCollision/NarrowPhaseCollision/btGjkPairDetector.h>
#include <BulletCollision/NarrowPhaseCollision/btPointCollector.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <btBulletDynamicsCommon.h>

void bt_body_config(
		BtWorlds *p_worlds,
		Query<
				Any<Changed<BtRigidBody>,
						Changed<BtWorldMarker>,
						Join<
								Changed<BtShapeBox>,
								Changed<BtShapeSphere>>>> &p_query) {
	for (auto [body, world_marker, shape_container] : p_query) {
		if (body == nullptr) {
			// Body not yet assigned, skip this.
			continue;
		}

		// Config shape.
		BtRigidShape *shape = shape_container.as<BtRigidShape>();
		if (shape != nullptr) {
			if (body->get_shape() != shape->get_shape()) {
				// Body shape is different (or nullptr) form the shape, assign it.
				body->set_shape(shape->get_shape());
			}
		} else {
			if (body->get_shape() != nullptr) {
				// Body has something, but the shape is null, so unassign it.
				body->set_shape(nullptr);
			}
		}

		// Search the world
		const BtWorldIndex world_index = world_marker != nullptr ? static_cast<BtWorldIndex>(world_marker->world_index) : BT_WORLD_0;
		if (body->get_current_world() != world_index) {
			// Refresh world
			body->set_current_world(world_index);
		}

		// Reload mass
		if (body->need_mass_reload() && body->get_shape() != nullptr) {
			body->reload_mass(body->get_shape());
		}

		// Reload world
		if (body->need_body_reload() && p_worlds != nullptr) {
			// TODO support world initialization when the body want to stay in
			// another world?
			BtWorld *world = p_worlds->get_space(world_index);

			if (body->get_body_mode() == BtRigidBody::RIGID_MODE_STATIC) {
				world->get_dynamics_world()->addCollisionObject(
						body->get_body(),
						body->get_layer(),
						body->get_mask());
			} else {
				world->get_dynamics_world()->addRigidBody(
						body->get_body(),
						body->get_layer(),
						body->get_mask());
			}

			body->reload_body();
		}
	}
}
