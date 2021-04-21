#pragma once

#include "utilities.h"

class btDiscreteDynamicsWorld;
class BtSpace;

/// Performs a test motion.
/// @param p_collision_object is optional and can be `nullptr`. When set the test will ignore this body.
KinematicConvexQResult test_motion(
		const btCollisionObject *p_collision_object,
		const btConvexShape *p_shape,
		const btVector3 &p_position,
		const btVector3 &p_motion,
		real_t p_margin,
		int p_collision_mask,
		bool p_skip_if_moving_away,
		const BtSpace *p_space);

/// Performs a test motion from position to target location.
/// @param p_collision_object is optional and can be `nullptr`. When set the test will ignore this body.
KinematicConvexQResult test_motion_target(
		const btCollisionObject *p_collision_object,
		const btConvexShape *p_shape,
		const btVector3 &p_position,
		const btVector3 &p_target,
		real_t p_margin,
		int p_collision_mask,
		bool p_skip_if_moving_away,
		const BtSpace *p_space);

/// Performs a contact test for the given shape.
/// @param p_collision_object is optional and can be `nullptr`. When set the test will ignore this body.
///
/// TODO this function need mutable access to the world, which is bad.
/// TODO Can we change it?
KinematicContactQResult test_contact(
		const btCollisionObject *p_collision_object,
		btConvexShape *p_shape,
		const btVector3 &p_position,
		real_t p_margin,
		int p_collision_mask,
		bool p_smooth_results,
		BtSpace *p_space);

/// Perform a raycast.
/// @param p_collision_object is optional and can be `nullptr`. When set the test will ignore this body.
KinematicRayQResult test_ray(
		const btCollisionObject *p_collision_object,
		const btVector3 &p_from,
		const btVector3 &p_to,
		int p_collision_mask,
		const BtSpace *p_space);
