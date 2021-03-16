#pragma once

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
				Any<Changed<BtRigidBody>,
						Changed<BtWorldMarker>,
						Join<
								Changed<BtShapeBox>,
								Changed<BtShapeSphere>>>> &p_query);

// TODO Shape remove from `Entity`

// TODO Body remove from `Entity`
