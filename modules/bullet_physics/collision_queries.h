#pragma once

#include "utilities.h"

class btDiscreteDynamicsWorld;
class BtSpace;

/// Performs a test motion.
/// @param p_collision_object is optional and can be `nullptr`. When set the test will ignore this body.
KinematicConvexQResult test_motion(
		const BtSpace *p_space,
		const btCollisionObject *p_collision_object,
		const btConvexShape *p_shape,
		const Vector3 &p_position,
		const Vector3 &p_motion,
		real_t p_margin,
		int p_collision_mask,
		bool p_skip_if_moving_away);

/// Performs a test motion.
/// @param p_collision_object is optional and can be `nullptr`. When set the test will ignore this body.
BtKinematicConvexQResult test_motion(
		const BtSpace *p_space,
		const btCollisionObject *p_collision_object,
		const btConvexShape *p_shape,
		const btVector3 &p_position,
		const btVector3 &p_motion,
		real_t p_margin,
		int p_collision_mask,
		bool p_skip_if_moving_away);

/// Performs a test motion from position to target location.
/// @param p_collision_object is optional and can be `nullptr`. When set the test will ignore this body.
KinematicConvexQResult test_motion_target(
		const BtSpace *p_space,
		const btCollisionObject *p_collision_object,
		const btConvexShape *p_shape,
		const Vector3 &p_position,
		const Vector3 &p_target,
		real_t p_margin,
		int p_collision_mask,
		bool p_skip_if_moving_away);

/// Performs a test motion from position to target location.
/// @param p_collision_object is optional and can be `nullptr`. When set the test will ignore this body.
BtKinematicConvexQResult test_motion_target(
		const BtSpace *p_space,
		const btCollisionObject *p_collision_object,
		const btConvexShape *p_shape,
		const btVector3 &p_position,
		const btVector3 &p_target,
		real_t p_margin,
		int p_collision_mask,
		bool p_skip_if_moving_away);

/// Performs a contact test for the given shape.
/// @param p_collision_object is optional and can be `nullptr`. When set the test will ignore this body.
///
/// TODO this function need mutable access to the world, which is bad.
/// TODO Can we change it?
KinematicContactQResult test_contact(
		BtSpace *p_space,
		const btCollisionObject *p_collision_object,
		btConvexShape *p_shape,
		const Vector3 &p_position,
		real_t p_margin,
		int p_collision_mask,
		bool p_smooth_results);

/// Performs a contact test for the given shape.
/// @param p_collision_object is optional and can be `nullptr`. When set the test will ignore this body.
///
/// TODO this function need mutable access to the world, which is bad.
/// TODO Can we change it?
BtKinematicContactQResult test_contact(
		BtSpace *p_space,
		const btCollisionObject *p_collision_object,
		btConvexShape *p_shape,
		const btVector3 &p_position,
		real_t p_margin,
		int p_collision_mask,
		bool p_smooth_results);

/// Perform a raycast.
/// @param p_collision_object is optional and can be `nullptr`. When set the test will ignore this body.
KinematicRayQResult test_ray(
		const BtSpace *p_space,
		const btCollisionObject *p_collision_object,
		const Vector3 &p_from,
		const Vector3 &p_to,
		int p_collision_mask);

/// Perform a raycast.
/// @param p_collision_object is optional and can be `nullptr`. When set the test will ignore this body.
BtKinematicRayQResult test_ray(
		const BtSpace *p_space,
		const btCollisionObject *p_collision_object,
		const btVector3 &p_from,
		const btVector3 &p_to,
		int p_collision_mask);
