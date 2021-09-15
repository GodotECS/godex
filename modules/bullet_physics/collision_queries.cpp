#include "collision_queries.h"

#include "bullet_types_converter.h"
#include "databag_space.h"
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <btBulletCollisionCommon.h>

KinematicConvexQResult test_motion(
		const BtSpace *p_space,
		const btCollisionObject *p_collision_object,
		const btConvexShape *p_shape,
		const Vector3 &p_position,
		const Vector3 &p_motion,
		real_t p_margin,
		int p_collision_mask,
		bool p_skip_if_moving_away) {
	btVector3 bt_position;
	btVector3 bt_motion;
	G_TO_B(p_position, bt_position);
	G_TO_B(p_motion, bt_motion);

	const BtKinematicConvexQResult bt_res = test_motion(
			p_space,
			p_collision_object,
			p_shape,
			bt_position,
			bt_motion,
			p_margin,
			p_collision_mask,
			p_skip_if_moving_away);

	KinematicConvexQResult res;
	res.closest_hit_fraction = bt_res.m_closestHitFraction;
	B_TO_G(bt_res.hit_normal, res.hit_normal);
	B_TO_G(bt_res.m_hitPointWorld, res.hit_point);
	res.hit_collision_object = bt_res.hit_collision_object;
	return res;
}

BtKinematicConvexQResult test_motion(
		const BtSpace *p_space,
		const btCollisionObject *p_collision_object,
		const btConvexShape *p_shape,
		const btVector3 &p_position,
		const btVector3 &p_motion,
		real_t p_margin,
		int p_collision_mask,
		bool p_skip_if_moving_away) {
	BtKinematicConvexQResult result(
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
		const Vector3 &p_position,
		const Vector3 &p_target,
		real_t p_margin,
		int p_collision_mask,
		bool p_skip_if_moving_away) {
	btVector3 bt_position;
	btVector3 bt_target;
	G_TO_B(p_position, bt_position);
	G_TO_B(p_target, bt_target);

	const BtKinematicConvexQResult bt_res = test_motion(
			p_space,
			p_collision_object,
			p_shape,
			bt_position,
			bt_target,
			p_margin,
			p_collision_mask,
			p_skip_if_moving_away);

	KinematicConvexQResult res;
	res.closest_hit_fraction = bt_res.m_closestHitFraction;
	B_TO_G(bt_res.hit_normal, res.hit_normal);
	B_TO_G(bt_res.m_hitPointWorld, res.hit_point);
	res.hit_collision_object = bt_res.hit_collision_object;
	return res;
}

BtKinematicConvexQResult test_motion_target(
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
		const Vector3 &p_position,
		real_t p_margin,
		int p_collision_mask,
		bool p_smooth_results) {
	btVector3 bt_position;
	G_TO_B(p_position, bt_position);

	const BtKinematicContactQResult bt_res = test_contact(
			p_space,
			p_collision_object,
			p_shape,
			bt_position,
			p_margin,
			p_collision_mask,
			p_smooth_results);

	// TODO worth this implementation, or better use `result_count` insted?
	KinematicContactQResult res;
	for (uint32_t i = 0; i < KINEMATIC_CONTACT_MAX_RESULTS; i += 1) {
		res.results[i].distance = bt_res.results[i].distance;
		B_TO_G(bt_res.results[i].hit_normal, res.results[i].hit_normal);
		B_TO_G(bt_res.results[i].position, res.results[i].hit_point);
		res.results[i].hit_collision_object = bt_res.results[i].object;
	}
	res.result_count = bt_res.result_count;
	return res;
}

BtKinematicContactQResult test_contact(
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
	BtKinematicContactQResult result(p_collision_object, &query_collision_object);
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
		const Vector3 &p_from,
		const Vector3 &p_to,
		int p_collision_mask) {
	btVector3 bt_from;
	btVector3 bt_to;
	G_TO_B(p_from, bt_from);
	G_TO_B(p_to, bt_to);

	const BtKinematicRayQResult bt_res = test_ray(
			p_space,
			p_collision_object,
			bt_from,
			bt_to,
			p_collision_mask);

	KinematicRayQResult res;
	B_TO_G(bt_res.m_hitNormalWorld, res.hit_normal);
	B_TO_G(bt_res.m_hitPointWorld, res.hit_point);
	res.closest_hit_fraction = bt_res.m_closestHitFraction;
	res.collision_object = bt_res.m_collisionObject;
	return res;
}

BtKinematicRayQResult test_ray(
		const BtSpace *p_space,
		const btCollisionObject *p_collision_object,
		const btVector3 &p_from,
		const btVector3 &p_to,
		int p_collision_mask) {
	BtKinematicRayQResult result(p_collision_object, p_from, p_to);

	result.m_collisionFilterGroup = 0;
	result.m_collisionFilterMask = p_collision_mask;

	p_space->get_dynamics_world()->rayTest(p_from, p_to, result);
	return result;
}
