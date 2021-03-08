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
	ECS::register_system(bt_config_shape, "BtConfigShape", "Bullet Physics - Configure shape");
	ECS::register_system(bt_config_body, "BtConfigBody", "Bullet Physics - Configure body");
	ECS::register_system(bt_config_body_world, "BtConfigBodyWorld", "Bullet Physics - Put the body inside the proper Physical World.");
}

void ecs_unregister_bullet_physics_types() {
}
