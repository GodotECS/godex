#pragma once

#include <BulletCollision/BroadphaseCollision/btBroadphaseProxy.h>
#include <LinearMath/btTransform.h>

class btCollisionShape;

typedef bool (*OverlappingFunc)(
		btCollisionShape *p_shape_1,
		const btTransform &p_shape_1_transform,
		btCollisionShape *p_shape_2,
		const btTransform &p_shape_2_transform);

/// Check if two shapes are overlapping each other. The algorithm used are a
/// mix of SAT and some accelerated one.
/// The accelerated checks are implemented for:
/// - Sphere <--> Sphere
/// - Sphere <--> Box
/// - Sphere <--> Capsule
/// - Capsule <--> Capsule
struct OverlapCheck {
	static OverlappingFunc overlapping_funcs[MAX_BROADPHASE_COLLISION_TYPES][MAX_BROADPHASE_COLLISION_TYPES];

	static void init();
	static OverlappingFunc find_algorithm(int body_1, int body_2);
};
