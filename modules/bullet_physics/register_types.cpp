#include "register_types.h"

#include "../../ecs.h"
#include "../godot/editor_plugins/components_gizmo_3d.h"
#include "bt_systems.h"
#include "components_gizmos.h"
#include "components_rigid_body.h"
#include "components_rigid_shape.h"
#include "databag_space.h"

void ecs_register_bullet_physics_types() {
	ECS::register_databag<BtPhysicsSpaces>();

	ECS::register_component<BtSpaceMarker>();
	ECS::register_component<BtRigidBody>();

	// Shapes
	ECS::register_component<BtShapeBox>();
	ECS::register_component<BtShapeSphere>();
	ECS::register_component<BtShapeCapsule>();
	ECS::register_component<BtShapeCone>();
	ECS::register_component<BtShapeCylinder>();
	ECS::register_component<BtShapeWorldMargin>();
	ECS::register_component<BtShapeConvex>();
	ECS::register_component<BtShapeTrimesh>();

	// Register `System`s
	ECS::register_system(bt_body_config, "BtBodyConfig", "Bullet Physics - Manage the lifetime of the Bodies");
	ECS::register_system(bt_spaces_step, "BtSpacesStep", "Bullet Physics - Steps the physics spaces.");
	ECS::register_system(bt_body_sync, "BtBodySync", "Bullet Physics - Read the Physics Engine and update the Bodies");
	//ECS::register_system(bt_area_sync, "BtBodySync", "Bullet Physics - Read the Physics Engine and update the Bodies");
	//ECS::register_system(bt_area_sync, "BtBodySync", "Bullet Physics - Read the Physics Engine and update the Bodies");

	Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtShapeComponentsGizmo));
}

void ecs_unregister_bullet_physics_types() {
}
