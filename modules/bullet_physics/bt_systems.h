#pragma once

#include "../godot/components/transform_component.h"
#include "../godot/databags/godot_engine_databags.h"
#include "components_area.h"
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
								Changed<BtShapeBox>,
								Changed<BtShapeSphere>,
								Changed<BtShapeCapsule>,
								Changed<BtShapeCone>,
								Changed<BtShapeCylinder>,
								Changed<BtShapeWorldMargin>,
								Changed<BtShapeConvex>,
								Changed<BtShapeTrimesh>>>,
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
								Changed<BtShapeBox>,
								Changed<BtShapeSphere>,
								Changed<BtShapeCapsule>,
								Changed<BtShapeCone>,
								Changed<BtShapeCylinder>,
								Changed<BtShapeWorldMargin>,
								Changed<BtShapeConvex>,
								Changed<BtShapeTrimesh>>>,
				Maybe<TransformComponent>> &p_query);

// TODO Shape remove from `Entity`

// TODO Body remove from `Entity`

void bt_spaces_step(
		BtPhysicsSpaces *p_spaces,
		const FrameTime *p_iterator_info,
		// TODO this is not used, though we need it just to be sure they are not
		// touched by anything else.
		Query<BtRigidBody, BtShapeBox, BtShapeSphere, BtShapeCapsule, BtShapeCone, BtShapeCylinder, BtShapeWorldMargin, BtShapeConvex, BtShapeTrimesh> &p_query);

void bt_body_sync(
		BtPhysicsSpaces *p_spaces,
		Query<BtRigidBody, TransformComponent> &p_query);
