#include "collision_queries.h"

#include "databag_space.h"
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <btBulletCollisionCommon.h>

KinematicConvexQResult test_motion(
		const BtSpace *p_space,
		const btCollisionObject *p_collision_object,
		const btConvexShape *p_shape,
		const btVector3 &p_position,
		const btVector3 &p_motion,
		real_t p_margin,
		int p_collision_mask,
		bool p_skip_if_moving_away) {
	KinematicConvexQResult result(
			p_collision_object,
			p_motion.isZero() ? btVector3(0.0, 0.0, 0.0) : p_motion.normalized(),
			p_skip_if_moving_away);

	ERR_FAIL_COND_V(p_shape == nullptr, result);

	result.m_collisionFilterGroup = 0;
	result.m_collisionFilterMask = p_collision_mask;

	p_space->get_dynamics_world()->convexSweepTest(
			p_shape,
			btTransform(btMatrix3x3::getIdentity(), p_position),
			btTransform(btMatrix3x3::getIdentity(), p_position + p_motion),
			result,
			p_margin);

	return result;
}

KinematicConvexQResult test_motion_target(
		const BtSpace *p_space,
		const btCollisionObject *p_collision_object,
		const btConvexShape *p_shape,
		const btVector3 &p_position,
		const btVector3 &p_target,
		real_t p_margin,
		int p_collision_mask,
		bool p_skip_if_moving_away) {
	return test_motion(
			p_space,
			p_collision_object,
			p_shape,
			p_position,
			p_target - p_position,
			p_margin,
			p_collision_mask,
			p_skip_if_moving_away);
}

KinematicContactQResult test_contact(
		BtSpace *p_space,
		const btCollisionObject *p_collision_object,
		btConvexShape *p_shape,
		const btVector3 &p_position,
		real_t p_margin,
		int p_collision_mask,
		bool p_smooth_results) {
	// Note: I'm not using the collision_object because I don't want to change
	// the main object transform. If turns out that this query is slow, we must
	// reconsider this.
	btCollisionObject query_collision_object;
	KinematicContactQResult result(p_collision_object, &query_collision_object);
	result.smooth_results = p_smooth_results;

	ERR_FAIL_COND_V(p_shape == nullptr, result);

	query_collision_object.setCollisionShape(p_shape);
	query_collision_object.setWorldTransform(btTransform(btMatrix3x3::getIdentity(), p_position));

	result.m_collisionFilterGroup = 0;
	result.m_collisionFilterMask = p_collision_mask;
	result.m_closestDistanceThreshold = p_margin;

	p_space->get_dynamics_world()->contactTest(
			&query_collision_object,
			result);

	return result;
}

KinematicRayQResult test_ray(
		const BtSpace *p_space,
		const btCollisionObject *p_collision_object,
		const btVector3 &p_from,
		const btVector3 &p_to,
		int p_collision_mask) {
	KinematicRayQResult result(p_collision_object, p_from, p_to);

	result.m_collisionFilterGroup = 0;
	result.m_collisionFilterMask = p_collision_mask;

	p_space->get_dynamics_world()->rayTest(p_from, p_to, result);
	return result;
}
