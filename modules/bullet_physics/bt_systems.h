#pragma once

#include "../godot/components/transform_component.h"
#include "../godot/databags/godot_engine_databags.h"
#include "components_rigid_body.h"
#include "components_rigid_shape.h"
#include "databag_world.h"

/// Configures the body.
/// This `System` is responsible for the body lifetime.
/// - Build the shape.
/// - Build the body.
/// - Assign the shape to the body.
/// - Assigne the Body to the world.
/// - Re-fresh the body if something changed.
void bt_body_config(
		BtWorlds *p_worlds,
		Query<
				EntityID,
				Any<Changed<BtRigidBody>,
						Changed<BtWorldMarker>,
						Join<
								Changed<BtShapeBox>,
								Changed<BtShapeSphere>>>,
				Maybe<TransformComponent>> &p_query);

// TODO Shape remove from `Entity`

// TODO Body remove from `Entity`

void bt_world_step(
		BtWorlds *p_worlds,
		const FrameTime *p_iterator_info,
		// TODO this is not used, though we need it just to be sure they are not
		// touched by anything else.
		Query<BtRigidBody, BtShapeBox, BtShapeSphere> &p_query);

void bt_body_sync(
		BtWorlds *p_worlds,
		Query<BtRigidBody, TransformComponent> &p_query);
