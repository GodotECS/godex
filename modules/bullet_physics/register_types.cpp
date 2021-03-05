#include "register_types.h"

#include "../../ecs.h"
#include "components_rigid_body.h"
#include "components_rigid_shape.h"
#include "space_databag.h"

void ecs_register_bullet_physics_types() {
	ECS::register_databag<BtSpaces>();

	ECS::register_component<BtWorldMarker>();
	ECS::register_component<BtRigidBody>();
	ECS::register_component<BtShapeBox>();
}

void ecs_unregister_bullet_physics_types() {
}
