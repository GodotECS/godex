#pragma once

#include "../godot/databags/godot_engine_databags.h"
#include "components_generic.h"
#include "components_rigid_body.h"
//#include "components_rigid_shape.h"
#include "databag_space.h"

void bt_walk(
		BtPhysicsSpaces *p_spaces,
		Query<BtRigidBody, WalkIntention> &p_query);
