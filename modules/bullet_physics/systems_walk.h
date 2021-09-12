#pragma once

#include "../../databags/frame_time.h"
#include "../godot/components/transform_component.h"
#include "components_generic.h"
#include "components_pawn.h"
#include "components_rigid_body.h"
#include "databag_space.h"
#include "shape_base.h"

void bt_pawn_walk(
		const FrameTime *frame_time,
		BtPhysicsSpaces *p_spaces, // TODO can this be const?
		Query<BtRigidBody, BtStreamedShape, BtPawn> &p_query);
