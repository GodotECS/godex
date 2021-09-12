#pragma once

#include "core/math/math_defs.h"
#include "core/math/vector3.h"
#include "core/string/ustring.h"
#include "core/templates/local_vector.h"
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <LinearMath/btVector3.h>

// TODO rename
// 0.5 Deg tolerance: RAD_TO_DEG(acos(0.01)) == 89.5 deg. Is valid only with det product.
#define HALF_DEG_TOLERANCE 0.01
#define KINEMATIC_CONTACT_MAX_RESULTS 6

/// Used to convert the `EntityID` from int to uint32_t.
/// This works only if `int` is represented as 32 bits, however this is a lot
/// common and we can assume it's safe.
union EntityIDConverter {
	int entity_id_int;
	uint32_t entity_id_uint;
};

/// Returns project p_vec along the perpendicular of the p_norm.
Vector3 vec_project_perpendicular(const Vector3 &p_vec, const Vector3 &p_normal);

/// Returns project p_vec along the perpendicular of the p_norm.
btVector3 vec_project_perpendicular(const btVector3 &p_vec, const btVector3 &p_normal);

/// Returns the p_vec project on the normal.
btVector3 vec_project(const btVector3 &a, const btVector3 &b);

// Reflects the vector along the plane normal.
btVector3 reflect(const btVector3 &p_vec, const btVector3 &p_normal);

// Bounce the vector along the plane normal.
btVector3 bounce(const btVector3 &p_vec, const btVector3 &p_normal);

String vtos(const btVector3 &p_vec);

struct KinematicConvexQResult {
	real_t closest_hit_fraction;
	Vector3 hit_normal;
	// Relative to world.
	Vector3 hit_point;
	const btCollisionObject *hit_collision_object;
};

struct BtKinematicConvexQResult : public btCollisionWorld::ConvexResultCallback {
	const btCollisionObject *m_self_object;
	const btVector3 motion_direction;
	const bool skip_if_moving_away;
	btVector3 hit_normal;
	btVector3 m_hitPointWorld;
	const btCollisionObject *hit_collision_object = nullptr;

	BtKinematicConvexQResult(const btCollisionObject *p_self_object, const btVector3 &p_motion_direction, bool p_skip_if_moving_away) :
			m_self_object(p_self_object),
			motion_direction(p_motion_direction),
			skip_if_moving_away(p_skip_if_moving_away) {}

	virtual bool needsCollision(btBroadphaseProxy *proxy0) const;
	virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult &convexResult, bool normalInWorldSpace);
};

struct KinematicContactQResult {
	struct Result {
		real_t distance;
		Vector3 hit_normal;
		// Relative to world.
		Vector3 hit_point;
		const btCollisionObject *hit_collision_object;
	};

	uint32_t result_count;
	Result results[KINEMATIC_CONTACT_MAX_RESULTS];
};

/// Takes the six deepest penetrations.
/// The results that have `distance` > `0.0` can be considered not penetratig.
/// Keep in mind that collision detection may not be precise at 100%, but
/// leaving `smooth_results` to true an extra check will run to smooth them out.
struct BtKinematicContactQResult : public btCollisionWorld::ContactResultCallback {
	struct Result {
		/// Negative, mean penetration.
		real_t distance = 100.0;
		btVector3 position;
		btVector3 hit_normal;
		const btCollisionObject *object = nullptr;
	};

	/// Used to avoid collision with sefl, can be null.
	const btCollisionObject *m_self_object;

	/// This must always be not null and must point to the collision object set
	/// in the query.
	const btCollisionObject *m_query_object;

	/// Contains the three most deepest contacts.
	uint32_t result_count = 0;
	Result results[KINEMATIC_CONTACT_MAX_RESULTS];
	LocalVector<Result> debug_results; // TODO remove this.

	/// Put to false if you don't need that the results are smoothed.
	/// If true, a new result is inserted only if there is not a similar one.
	bool smooth_results = true;

	BtKinematicContactQResult(const btCollisionObject *p_self_object, btCollisionObject *p_query_object) :
			m_self_object(p_self_object),
			m_query_object(p_query_object) {}

	virtual bool needsCollision(btBroadphaseProxy *proxy0) const;
	virtual btScalar addSingleResult(btManifoldPoint &cp, const btCollisionObjectWrapper *colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper *colObj1Wrap, int partId1, int index1);
};

struct KinematicRayQResult {
	Vector3 hit_normal;
	// Relative to world.
	Vector3 hit_point;
	real_t closest_hit_fraction;
	const btCollisionObject *collision_object;

	bool has_hit() const {
		return collision_object != nullptr;
	}
};

struct BtKinematicRayQResult : public btCollisionWorld::ClosestRayResultCallback {
	const btCollisionObject *m_self_object;

	BtKinematicRayQResult(
			const btCollisionObject *p_self,
			const btVector3 &rayFromWorld,
			const btVector3 &rayToWorld);

	virtual bool needsCollision(btBroadphaseProxy *proxy0) const;
};
