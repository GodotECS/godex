#include "register_types.h"

#include "../../ecs.h"
#include "../godot/editor_plugins/components_gizmo_3d.h"
#include "bt_systems.h"
#include "components_area.h"
#include "components_generic.h"
#include "components_gizmos.h"
#include "components_rigid_body.h"
#include "components_rigid_shape.h"
#include "databag_space.h"
#include "overlap_check.h"

void register_bullet_physics_types() {
	// Initialize the Overlap check algorithms.
	OverlapCheck::init();

	ECS::register_spawner<OverlapEventSpawner>();

	ECS::register_databag<BtPhysicsSpaces>();
	ECS::register_databag<BtCache>();

	ECS::register_component<BtSpaceMarker>();
	ECS::register_component<BtRigidBody>();
	ECS::register_component<BtArea>();

	// Shapes
	ECS::register_component<BtBox>();
	ECS::register_component<BtSphere>();
	ECS::register_component<BtCapsule>();
	ECS::register_component<BtCone>();
	ECS::register_component<BtCylinder>();
	//ECS::register_component<BtWorldMargin>();
	ECS::register_component<BtConvex>();
	ECS::register_component<BtTrimesh>();

	// Generics
	// TODO move this inside `modules/godot`?
	ECS::register_component<Force>();
	ECS::register_component<Torque>();
	ECS::register_component<Impulse>();
	ECS::register_component<TorqueImpulse>();

	// Register `System`s
	ECS::register_system(bt_body_config, "BtBodyConfig", "Bullet Physics - Manage the lifetime of the Bodies");
	ECS::register_system(bt_area_config, "BtAreaConfig", "Bullet Physics - Manage the lifetime of the Area");
	ECS::register_system(bt_apply_forces, "BtApplyForces", "Bullet Physics - Apply `Forces` and `Impulses` to bodies.");
	ECS::register_system(bt_spaces_step, "BtSpacesStep", "Bullet Physics - Steps the physics spaces.");
	ECS::register_system(bt_overlap_check, "BtOverlapCheck", "Bullet Physics - Allow the areas to detect ovelapped bodies.");
	ECS::register_system(bt_body_sync, "BtBodySync", "Bullet Physics - Read the Physics Engine and update the Bodies");

	// Register gizmos
	Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtBoxGizmo));
	Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtSphereGizmo));
	Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtCapsuleGizmo));
	Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtConeGizmo));
	Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtCylinderGizmo));
	Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtConvexGizmo));
	Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtTrimeshGizmo));
}

void unregister_bullet_physics_types() {
}
