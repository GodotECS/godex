#include "register_types.h"

#include "../../ecs.h"
#include "components.h"
#include "space_databag.h"

void ecs_register_bullet_physics_types() {
	ECS::register_databag<BtSpaces>();

	ECS::register_component<BtSpaceMarker>();
	ECS::register_component<BtRigidBody>();
}

void ecs_unregister_bullet_physics_types() {
}
