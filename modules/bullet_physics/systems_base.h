#pragma once

#include "../godot/components/transform_component.h"
#include "../godot/databags/godot_engine_databags.h"
#include "components_area.h"
#include "components_generic.h"
#include "components_pawn.h"
#include "components_rigid_body.h"
#include "components_rigid_shape.h"
#include "databag_space.h"

/// Configures the body.
/// This `System` is responsible for the body lifetime.
/// - Build the shape.
/// - Build the body.
/// - Assign the shape to the body.
/// - Assigne the Body to the world.
/// - Re-fresh the body if something changed.
void bt_body_config(
		BtPhysicsSpaces *p_spaces,
		Query<
				EntityID,
				Any<Changed<BtRigidBody>,
						Changed<BtSpaceMarker>,
						Join<
								Changed<BtBox>,
								Changed<BtSphere>,
								Changed<BtCapsule>,
								Changed<BtCone>,
								Changed<BtCylinder>,
								Changed<BtWorldMargin>,
								Changed<BtConvex>,
								Changed<BtTrimesh>,
								Changed<BtStreamedShape>>>,
				Maybe<TransformComponent>> &p_query);

/// Configures the Area
/// This `System` is responsible for the area lifetime.
/// - Build the shape.
/// - Build the area.
/// - Assign the shape to the area.
/// - Assigne the area to the world.
/// - Re-fresh the area if something changed.
void bt_area_config(
		BtPhysicsSpaces *p_spaces,
		Query<
				EntityID,
				Any<Changed<BtArea>,
						Changed<BtSpaceMarker>,
						Join<
								Changed<BtBox>,
								Changed<BtSphere>,
								Changed<BtCapsule>,
								Changed<BtCone>,
								Changed<BtCylinder>,
								Changed<BtWorldMargin>,
								Changed<BtConvex>,
								Changed<BtTrimesh>>>,
				Maybe<TransformComponent>> &p_query);

void bt_apply_forces(
		Query<BtRigidBody, Batch<Force>, Maybe<BtPawn>> &p_query_forces,
		Query<BtRigidBody, Batch<Torque>> &p_query_torques,
		Query<BtRigidBody, Batch<Impulse>, Maybe<BtPawn>> &p_query_impulses,
		Query<BtRigidBody, Batch<TorqueImpulse>> &p_query_torque_impulses,
		Storage<Impulse> *p_inpulses,
		Storage<TorqueImpulse> *p_torque_inpulses);

// TODO Shape remove from `Entity`

// TODO Body remove from `Entity`

void bt_spaces_step(
		BtPhysicsSpaces *p_spaces,
		const FrameTime *p_iterator_info,
		// TODO this is not used, though we need it just to be sure they are not
		// touched by anything else.
		Query<BtRigidBody, BtArea, BtBox, BtSphere, BtCapsule, BtCone, BtCylinder, BtWorldMargin, BtConvex, BtTrimesh> &p_query);

/// Perform the Areas overlap check.
void bt_overlap_check(
		const BtPhysicsSpaces *p_spaces,
		BtCache *p_cache,
		Spawner<OverlapEventSpawner> &p_spawner,
		Query<EntityID, BtArea> &p_query);

void bt_body_sync(
		BtPhysicsSpaces *p_spaces,
		Query<BtRigidBody, TransformComponent> &p_query);
