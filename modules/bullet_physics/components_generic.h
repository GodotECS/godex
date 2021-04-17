#pragma once

#include "../../components/component.h"
#include "../../storage/dense_vector.h"
#include "../../storage/dense_vector_storage.h"
#include "components_area.h"

struct Force {
	COMPONENT_BATCH(Force, DenseVector, -1)
	SPAWNERS(OverlapEventSpawner)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	Vector3 location;
	Vector3 force;
};

struct Torque {
	COMPONENT_BATCH(Torque, DenseVector, -1)
	SPAWNERS(OverlapEventSpawner)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	Vector3 torque;
};

struct Impulse {
	COMPONENT_BATCH(Impulse, DenseVector, -1)
	SPAWNERS(OverlapEventSpawner)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	Vector3 location;
	Vector3 impulse;
};

struct TorqueImpulse {
	COMPONENT_BATCH(TorqueImpulse, DenseVector, -1)
	SPAWNERS(OverlapEventSpawner)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	Vector3 impulse;
};

/// The WalkIntention is a component that allow a Kienamtic Body to walk around:
/// up and down slopes, move up and down stairs.
///
/// # How to use it
/// To make the body walk, you just need to set the velocity, from a system
/// that does it according the Player inputs.
///
/// # Forces & Impulese
/// Forces and Impulses are also taken into account.
///
/// # Reference plane
/// It's possible to change the walking algorithm reference plane. This feature
/// is a lot useful when you want that your Pawn walks all around a sphere.
struct WalkIntention {
	COMPONENT(WalkIntention, DenseVectorStorage)

	static void _bind_methods();

	/// Current pawn linear velocity and direction.
	Vector3 velocity;

	/// The step height the WalkIntention will be able to step up.
	real_t step_height = 0.2;

	/// The ground direction the WalkIntention will be computed.
	Basis ground_direction;

	/// Control if the WalkIntention should snap to the ground. Set this to false, when
	/// your pawn is falling or jumping, so to have a more natural motion.
	bool snap_to_ground = true;
};
