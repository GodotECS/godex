#pragma once

#include "components_rigid_body.h"
#include "components_rigid_shape.h"
#include "databag_world.h"

/// Associate the Body to a Shape if both are part of the `Entity`.
void bt_config_shape(
		Query<
				Changed<BtRigidBody>,
				Flatten<
						Changed<BtShapeBox>,
						Changed<BtShapeSphere>>> &p_query);

/// Configures the body.
/// This `System` configures the body
void bt_config_body(
		BtWorld *p_space,
		Query<
				Changed<BtRigidBody>,
				Flatten<
						Changed<BtShapeBox>,
						Changed<BtShapeSphere>>> &p_query);

/// Body world
void bt_config_body_world(
		BtWorld *p_space,
		Query<MaybeChanged<BtRigidBody>, MaybeChanged<BtWorldMarker>> &p_query);

// TODO Shape remove from `Entity`

// TODO Body remove from `Entity`
