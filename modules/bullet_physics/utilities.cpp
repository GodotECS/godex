
#include "utilities.h"

#include "bullet_result_callbacks.h"
#include "core/math/math_defs.h"

// TODO parametize?
// 5mm
#define SMOOTH_RESULTS_DISTANCE_TOLERANCE 0.05

/// Returns project p_vec along the perpendicular of the p_norm.
Vector3 vec_project_perpendicular(const Vector3 &p_vec, const Vector3 &p_normal) {
	const real_t dot = p_vec.dot(p_normal);
	const Vector3 ret = p_vec - p_normal * dot;
	return ret;
}

/// Returns project p_vec along the perpendicular of the p_norm.
btVector3 vec_project_perpendicular(const btVector3 &p_vec, const btVector3 &p_normal) {
	const real_t dot = p_vec.dot(p_normal);
	const btVector3 ret = p_vec - p_normal * dot;
	return ret;
}

btVector3 vec_project(const btVector3 &a, const btVector3 &b) {
#ifdef MATH_CHECKS
	CRASH_COND(b.isZero());
#endif
	return b * (a.dot(b) / b.length2());
}

btVector3 reflect(const btVector3 &p_vec, const btVector3 &p_normal) {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(ABS(p_normal.length() - 1.0) > CMP_EPSILON, btVector3(), "The normal btVector3 must be normalized.");
#endif
	return 2.0 * p_normal * p_vec.dot(p_normal) - p_vec;
}

btVector3 bounce(const btVector3 &p_vec, const btVector3 &p_normal) {
	return -reflect(p_vec, p_normal);
}

String vtos(const btVector3 &p_vec) {
	return String("[X: ") + rtos(p_vec[0]) + " Y: " + rtos(p_vec[1]) + " Z: " + rtos(p_vec[2]) + "]";
}

class GodexBtPlane {
	btVector3 normal;
	real_t distance;

public:
	GodexBtPlane(const btVector3 &p_point, const btVector3 &p_normal) {
		normal = p_normal;
		distance = normal.dot(p_point);
	}

	/// Retuns a positive distance between the point and it's plane projection.
	real_t distance_to(btVector3 p_other_point) const {
		return ABS(normal.dot(p_other_point) - distance);
	}
};

bool BtKinematicConvexQResult::needsCollision(btBroadphaseProxy *proxy0) const {
	btCollisionObject *oco = static_cast<btCollisionObject *>(proxy0->m_clientObject);
	if (m_self_object != nullptr) {
		if (oco == m_self_object) {
			// Never collide with self.
			return false;
		}
		if (!m_self_object->checkCollideWith(oco) || !oco->checkCollideWith(m_self_object)) {
			return false;
		}
	}
	if (oco->getInternalType() == btCollisionObject::CO_GHOST_OBJECT) {
		// Never collide with areas.
		return false;
	}
	return GodexBtFilterCallback::test_collision_filters(
			m_collisionFilterGroup,
			m_collisionFilterMask,
			proxy0->m_collisionFilterGroup,
			proxy0->m_collisionFilterMask);
}

btScalar BtKinematicConvexQResult::addSingleResult(
		btCollisionWorld::LocalConvexResult &convexResult,
		bool normalInWorldSpace) {
#ifdef TOOLS_ENABLED
	// Caller already does the filter on the m_closestHitFraction.
	CRASH_COND(convexResult.m_hitFraction > m_closestHitFraction);
#endif
	btVector3 norm;
	if (normalInWorldSpace) {
		norm = convexResult.m_hitNormalLocal;
	} else {
		// Transform normal into worldspace.
		norm = hit_collision_object->getWorldTransform().getBasis() * convexResult.m_hitNormalLocal;
	}

	if (skip_if_moving_away &&
			norm.dot(motion_direction) > -HALF_DEG_TOLERANCE) {
		// The object is moving away from this object, so don't stop.
		return 1.0;
	}

	m_closestHitFraction = convexResult.m_hitFraction;
	hit_collision_object = convexResult.m_hitCollisionObject;
	hit_normal = norm;
	m_hitPointWorld = convexResult.m_hitPointLocal;
	return convexResult.m_hitFraction;
}

bool BtKinematicContactQResult::needsCollision(btBroadphaseProxy *proxy0) const {
	btCollisionObject *oco = static_cast<btCollisionObject *>(proxy0->m_clientObject);
	if (m_self_object != nullptr) {
		if (oco == m_self_object) {
			// Never collide with self.
			return false;
		}
		if (!m_self_object->checkCollideWith(oco) || !oco->checkCollideWith(m_self_object)) {
			return false;
		}
	}
	if (oco->getInternalType() == btCollisionObject::CO_GHOST_OBJECT) {
		// Never collide with areas.
		return false;
	}
	return GodexBtFilterCallback::test_collision_filters(
			m_collisionFilterGroup,
			m_collisionFilterMask,
			proxy0->m_collisionFilterGroup,
			proxy0->m_collisionFilterMask);
}

btScalar BtKinematicContactQResult::addSingleResult(
		btManifoldPoint &cp,
		const btCollisionObjectWrapper *colObj0Wrap,
		int partId0,
		int index0,
		const btCollisionObjectWrapper *colObj1Wrap,
		int partId1,
		int index1) {
	const btVector3 norm = cp.m_normalWorldOnB * (m_query_object == colObj0Wrap->getCollisionObject() ? 1 : -1);
	btVector3 position = (m_query_object == colObj0Wrap->getCollisionObject() ? cp.getPositionWorldOnB() : cp.getPositionWorldOnA());
	const real_t distance = cp.getDistance() - m_closestDistanceThreshold;

	{
		// TODO this was for debug.
		BtKinematicContactQResult::Result debug_res;
		debug_res.distance = distance;
		debug_res.hit_normal = norm;
		debug_res.position = position;
		debug_res.object = m_query_object == colObj0Wrap->getCollisionObject() ? colObj1Wrap->getCollisionObject() : colObj0Wrap->getCollisionObject();
		debug_results.push_back(debug_res);
	}

	// TODO consider that we may want to take not the deepest but the one that appear more
	// or more sane (somehow)
	if (smooth_results) {
		LocalVector<uint32_t> discards;
		discards.reserve(KINEMATIC_CONTACT_MAX_RESULTS);

		const GodexBtPlane new_contact_plane(position, norm);
		for (uint32_t i = 0; i < result_count; i += 1) {
			// Checks if the contact is on the same plane.

			if (
					// Checks if the new contact is complanar with this other contact.
					new_contact_plane.distance_to(results[i].position) <= SMOOTH_RESULTS_DISTANCE_TOLERANCE ||
					// Checks if this other contact is complanar with the new contact.
					GodexBtPlane(results[i].position, results[i].hit_normal).distance_to(position) <= SMOOTH_RESULTS_DISTANCE_TOLERANCE) {
				// Check if this new contact is more penetrated.
				if (distance < results[i].distance) {
					// Yes it's more penetrated, so discard.
					discards.push_back(i);
				} else {
					// Ok the other one is better, so skip.
					results[i].position = position.lerp(results[i].position, 0.5);
					return 1.0;
				}

				position = position.lerp(results[i].position, 0.5);
			}
		}

		// Discard the contacs.
		for (uint32_t j = 0; j < discards.size(); j += 1) {
			result_count -= 1;

			for (uint32_t k = discards[j]; k < result_count; k += 1) {
				results[k] = results[k + 1];
			}

			// Also updates the remaining discards.
			for (uint32_t v = j + 1; v < discards.size(); v += 1) {
				if (discards[v] > discards[j]) {
					discards[v] -= 1;
				}
			}
		}
	}

	// This contact seems useful, so try to insert it.

	result_count = MIN(result_count + 1, KINEMATIC_CONTACT_MAX_RESULTS);

	uint32_t least_penetrated = 0;
	real_t dist = results[0].distance;
	for (uint32_t i = 1; i < result_count; i += 1) {
		if (results[i].distance > dist) {
			least_penetrated = i;
			dist = results[i].distance;
		}
	}

	results[least_penetrated].distance = distance;
	results[least_penetrated].hit_normal = norm;
	results[least_penetrated].position = position;
	results[least_penetrated].object = m_query_object == colObj0Wrap->getCollisionObject() ? colObj1Wrap->getCollisionObject() : colObj0Wrap->getCollisionObject();

	return results[least_penetrated].distance;
}

BtKinematicRayQResult::BtKinematicRayQResult(
		const btCollisionObject *p_self,
		const btVector3 &rayFrom,
		const btVector3 &rayTo) :
		ClosestRayResultCallback(rayFrom, rayTo),
		m_self_object(p_self) {
}

bool BtKinematicRayQResult::needsCollision(btBroadphaseProxy *proxy0) const {
	btCollisionObject *oco = static_cast<btCollisionObject *>(proxy0->m_clientObject);
	if (m_self_object != nullptr) {
		if (oco == m_self_object) {
			// Never collide with self.
			return false;
		}
		if (!m_self_object->checkCollideWith(oco) || !oco->checkCollideWith(m_self_object)) {
			return false;
		}
	}
	if (oco->getInternalType() == btCollisionObject::CO_GHOST_OBJECT) {
		// Never collide with areas.
		return false;
	}
	return GodexBtFilterCallback::test_collision_filters(
			m_collisionFilterGroup,
			m_collisionFilterMask,
			proxy0->m_collisionFilterGroup,
			proxy0->m_collisionFilterMask);
}
