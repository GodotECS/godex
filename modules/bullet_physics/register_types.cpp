#include "register_types.h"

#include "../../ecs.h"
#include "bt_systems.h"
#include "components_rigid_body.h"
#include "components_rigid_shape.h"
#include "databag_world.h"

void ecs_register_bullet_physics_types() {
	ECS::register_databag<BtWorlds>();

	ECS::register_component<BtWorldMarker>();
	ECS::register_component<BtRigidBody>();

	// Shapes
	ECS::register_component<BtShapeBox>();
	ECS::register_component<BtShapeSphere>();

	// Register `System`s
	ECS::register_system(bt_body_config, "BtBodyConfig", "Bullet Physics - Manage the lifetime of the Bodies");
}

void ecs_unregister_bullet_physics_types() {
}
