#include "register_types.h"

#include "../../ecs.h"
#include "../godot/editor_plugins/components_gizmo_3d.h"
#include "components_area.h"
#include "components_generic.h"
#include "components_gizmos.h"
#include "components_pawn.h"
#include "components_rigid_body.h"
#include "components_rigid_shape.h"
#include "databag_space.h"
#include "events_generic.h"
#include "overlap_check.h"
#include "systems_base.h"
#include "systems_walk.h"

void register_bullet_physics_types() {
	// Initialize the Overlap check algorithms.
	OverlapCheck::init();

	ECS::register_databag<BtPhysicsSpaces>();
	ECS::register_databag<BtCache>();

	ECS::register_component<BtSpaceMarker>();
	ECS::register_component<BtRigidBody>();
	ECS::register_component<BtArea>();
	ECS::register_component<BtPawn>();

	// Shapes
	ECS::register_component<BtBox>();
	ECS::register_component<BtSphere>();
	ECS::register_component<BtCapsule>();
	ECS::register_component<BtCone>();
	ECS::register_component<BtCylinder>();
	//ECS::register_component<BtWorldMargin>();
	ECS::register_component<BtConvex>();
	ECS::register_component<BtTrimesh>();
	ECS::register_component<BtStreamedShape>();

	ECS::register_event<OverlapStart>();
	ECS::register_event<OverlapEnd>();

	// Generics
	// TODO move this inside `modules/godot`?
	ECS::register_component<Force>();
	ECS::register_component<Torque>();
	ECS::register_component<Impulse>();
	ECS::register_component<TorqueImpulse>();

	// Register Base `System`s
	ECS::register_system_bundle("Bullet Physics Base Only")
			.set_description("System that allow to use the Bullet physics components.")
			.add(ECS::register_system(bt_body_config, "BtBodyConfig")
							.execute_in(PHASE_CONFIG, "Physics")
							.set_description("Bullet Physics - Manage the lifetime of the Bodies."))

			.add(ECS::register_system(bt_area_config, "BtAreaConfig")
							.execute_in(PHASE_CONFIG, "Physics")
							.set_description("Bullet Physics - Manage the lifetime of the Area."))

			.add(ECS::register_system(bt_apply_forces, "BtApplyForces")
							.execute_in(PHASE_PROCESS, "Physics")
							.set_description("Bullet Physics - Apply `Forces` and `Impulses` to bodies.")
							.after("CallPhysicsProcess"))

			.add(ECS::register_system(bt_spaces_step, "BtSpacesStep")
							.execute_in(PHASE_PROCESS, "Physics")
							.set_description("Bullet Physics - Steps the physics spaces.")
							.after("BtApplyForces"))

			.add(ECS::register_system(bt_overlap_check, "BtOverlapCheck")
							.execute_in(PHASE_POST_PROCESS, "Physics")
							.set_description("Bullet Physics - Allow the areas to detect ovelapped bodies."))

			.add(ECS::register_system(bt_body_sync, "BtBodySync")
							.execute_in(PHASE_POST_PROCESS, "Physics")
							.set_description("Bullet Physics - Read the Physics Engine and update the Bodies."))

			.add(ECS::register_system(bt_suppress_changed_warning, "bt_suppress_changed_warning")
							.execute_in(PHASE_MAX, "Physics")
							.set_description("Bullet Physics - Warning suppressor, does nothing and it's not included in the processing.")
							.with_flags(EXCLUDE_PIPELINE_COMPOSITION));

	// Register Walk `System`s
	ECS::register_system(bt_pawn_walk, "BtPawnWalk")
			.execute_in(PHASE_PROCESS, "Physics")
			.set_description("Bullet Physics - Make the Rigidbody in kinematic mode walk according to Pawn settings.")
			.after("BtApplyForces")
			.before("BtSpacesStep");

	ECS::register_system_bundle("Bullet Physics All Features")
			.add("BtBodyConfig")
			.add("BtAreaConfig")
			.add("BtApplyForces")
			.add("BtPawnWalk")
			.add("BtSpacesStep")
			.add("BtOverlapCheck")
			.add("BtBodySync")
			.add("bt_suppress_changed_warning");

	// Register gizmos
	Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtBoxGizmo));
	Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtSphereGizmo));
	Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtCapsuleGizmo));
	Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtConeGizmo));
	Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtCylinderGizmo));
	Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtConvexGizmo));
	Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtTrimeshGizmo));
	Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtPawnGizmo));
}

void unregister_bullet_physics_types() {
}
