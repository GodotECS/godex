#include "bt_systems.h"

#include "modules/bullet/bullet_types_converter.h"
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
		BtPhysicsSpaces *p_spaces,
		Query<
				EntityID,
				Any<Changed<BtRigidBody>,
						Changed<BtSpaceMarker>,
						Join<
								Changed<BtShapeBox>,
								Changed<BtShapeSphere>>>,
				Maybe<TransformComponent>> &p_query) {
	for (auto [entity, body, space_marker, shape_container, transform] : p_query) {
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

		// Reload mass
		if (body->need_mass_reload() && body->get_shape() != nullptr) {
			body->reload_mass(body->get_shape());
		}

		// Take the space this body should be on.
		const BtSpaceIndex space_index = body->get_shape() == nullptr ? BT_SPACE_NONE : (space_marker != nullptr ? static_cast<BtSpaceIndex>(space_marker->space_index) : BT_SPACE_0);

		// Reload space
		if ((body->need_body_reload() ||
					body->__current_space != space_index) &&
				p_spaces != nullptr) {
			// This body needs a realod.

			if (body->__current_space != BtSpaceIndex::BT_SPACE_NONE) {
				// Assume the space is the body is currently on is initialized.
				if (body->__current_mode == BtRigidBody::RIGID_MODE_STATIC) {
					p_spaces->get_space(body->__current_space)->get_dynamics_world()->removeCollisionObject(body->get_body());
				} else {
					p_spaces->get_space(body->__current_space)->get_dynamics_world()->removeRigidBody(body->get_body());
				}
			}

			// Now let's add it inside the space again.

			// TODO support space initialization when the body want to stay in
			// another space?
			BtSpace *space = p_spaces->get_space(space_index);

			if (body->get_body_mode() == BtRigidBody::RIGID_MODE_STATIC) {
				space->get_dynamics_world()->addCollisionObject(
						body->get_body(),
						body->get_layer(),
						body->get_mask());
			} else {
				space->get_dynamics_world()->addRigidBody(
						body->get_body(),
						body->get_layer(),
						body->get_mask());
			}

			body->__current_mode = body->get_body_mode();

			// Set transfrorm
			if (transform != nullptr) {
				btTransform t;
				G_TO_B(transform->transform, t);
				body->get_body()->setWorldTransform(t);
				body->get_motion_state()->transf = t;
			}

			body->get_body()->setMotionState(body->get_motion_state());
			body->get_motion_state()->entity = entity;
			body->get_motion_state()->space = space;
			body->reload_body(space_index);
		}
	}
}

void bt_spaces_step(
		BtPhysicsSpaces *p_spaces,
		const FrameTime *p_iterator_info,
		// TODO this is not used, though we need it just to be sure they are not
		// touched by anything else.
		Query<BtRigidBody, BtShapeBox, BtShapeSphere> &p_query) {
	const real_t physics_delta = p_iterator_info->get_physics_delta();

	// TODO consider to create a system for each space? So it has much more control.
	for (uint32_t i = 0; i < BtSpaceIndex::BT_SPACE_MAX; i += 1) {
		BtSpaceIndex w_i = (BtSpaceIndex)i;

		if (p_spaces->get_space(w_i)->get_dispatcher() == nullptr) {
			// This space is disabled.
			continue;
		}

		// Step bullet physics.
		p_spaces->get_space(w_i)->get_dynamics_world()->stepSimulation(
				physics_delta,
				0,
				0);
	}
}

void bt_body_sync(
		BtPhysicsSpaces *p_spaces,
		Query<BtRigidBody, TransformComponent> &p_query) {
	for (uint32_t i = 0; i < BtSpaceIndex::BT_SPACE_MAX; i += 1) {
		BtSpaceIndex w_i = (BtSpaceIndex)i;

		if (p_spaces->get_space(w_i)->get_dispatcher() == nullptr) {
			// This space is disabled.
			continue;
		}

		p_spaces->get_space(w_i)->moved_bodies.for_each([&](EntityID p_entity_id) {
			if (p_query.has(p_entity_id)) {
				auto [body, transform] = p_query.space(GLOBAL)[p_entity_id];
				B_TO_G(body->get_motion_state()->transf, transform->transform);
			}
		});

		p_spaces->get_space(w_i)->moved_bodies.clear();
	}
}
