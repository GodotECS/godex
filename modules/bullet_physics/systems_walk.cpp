#include "systems_walk.h"

#include "modules/bullet/bullet_types_converter.h"
#include <BulletCollision/BroadphaseCollision/btBroadphaseProxy.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <btBulletDynamicsCommon.h>

/*
btVector3 unstuck(
		const ShapeHolder &p_shape,
		btVector3 &r_position,
		const btVector3 &p_up_dir,
		real_t p_margin,
		real_t p_unstuck_factor // TODO do I need this, now?
) const {
#ifdef TOOLS_ENABLED
	// This function is not supposed to be called with null shpae.
	CRASH_COND(p_shape.margin_shape == nullptr);
#endif

	const btVector3 initial = r_position;

	for (int x = 0; x < UNSTUCK_TESTS; x += 1) {
		const KinematicContactQResult result = test_contact(p_shape.margin_shape, r_position, p_margin, true);

		btVector3 norm(0.0, 0.0, 0.0);
		real_t depth = 0.0;

		if (result.result_count <= 0) {
			// Shortcut when no collision is found.
			break;
		}

		// Combine the normal.
		for (uint32_t i = 0; i < result.result_count; i += 1) {
			if (unlikely(result.results[i].distance > 0.0)) {
				continue;
			}

			const real_t dot = result.results[i].normal.dot(p_up_dir);
			if (dot >= HALF_DEG_TOLERANCE * 4.0) {
				// Any bottom collision produces a depenetration toward up.
				// This allows to no slide downhil.
				norm += p_up_dir;
			} else {
				// All others, produces depenetration toward the normal.
				norm += result.results[i].normal;
			}
		}

		if (norm.length2() <= 0.0) {
			break;
		}

		// Along this normal takes the bigger penetration.
		norm.normalize();

		for (uint32_t i = 0; i < result.result_count; i += 1) {
			if (unlikely(result.results[i].distance > 0.0)) {
				continue;
			}

			const real_t equality = norm.dot(result.results[i].normal);
			const real_t new_depth = ((-result.results[i].distance) * equality) * UNSTUCK_FACTOR;
			if (new_depth > depth) {
				depth = new_depth;
			}
		}

		// Apply the penetration.
		if (depth == 0.0) {
			// Shortcut when no depenetration is performed.
			break;
		} else {
			r_position += norm * depth * p_unstuck_factor;
		}
	}

	// Computes the unstuck movement normal.
	const btVector3 n = r_position - initial;
	if (n.length2() > 0.0) {
		return n.normalized();
	} else {
		return btVector3(0.0, 0.0, 0.0);
	}
}
*/

void bt_walk(
		BtPhysicsSpaces *p_spaces,
		Query<BtRigidBody, BtPawn> &p_query) {
	for (auto [rigid_body, walk] : p_query) {
		ERR_CONTINUE_MSG(rigid_body->get_body_mode() != BtRigidBody::RIGID_MODE_KINEMATIC, "The mode of this body is not KINEMATIK");
	}
}
