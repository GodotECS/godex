#include "register_types.h"

#include "../../ecs.h"
#include "../godot/editor_plugins/components_gizmo_3d.h"
#include "components_area.h"
#include "components_generic.h"
#ifdef TOOLS_ENABLED
#include "editor/components_gizmos.h"
#endif
#include "components_pawn.h"
#include "components_rigid_body.h"
#include "databag_space.h"
#include "events_generic.h"
#include "overlap_check.h"
#include "shape_base.h"
#include "shape_box.h"
#include "shape_capsule.h"
#include "shape_convex.h"
#include "shape_cylinder.h"
#include "shape_sphere.h"
#include "shape_trimesh.h"
#include "systems_base.h"
#include "systems_walk.h"

void initialize_bullet_physics_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
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
		ECS::register_databag<BtShapeStorageBox>();

		ECS::register_component<BtSphere>();
		ECS::register_databag<BtShapeStorageSphere>();

		ECS::register_component<BtCapsule>();
		ECS::register_databag<BtShapeStorageCapsule>();

		ECS::register_component<BtCone>();
		ECS::register_databag<BtShapeStorageCone>();

		ECS::register_component<BtCylinder>();
		ECS::register_databag<BtShapeStorageCylinder>();

		ECS::register_component<BtConvex>();
		ECS::register_databag<BtShapeStorageConvex>();

		ECS::register_component<BtTrimesh>();
		ECS::register_databag<BtShapeStorageTrimesh>();

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
				.add(ECS::register_system(bt_config_body, "BtConfigBody")
								.execute_in(PHASE_CONFIG, "Physics")
								.set_description("Bullet Physics - Manage the lifetime of the Bodies."))

				.add(ECS::register_system(bt_config_area, "BtConfigArea")
								.execute_in(PHASE_CONFIG, "Physics")
								.set_description("Bullet Physics - Manage the lifetime of the Area."))

				.add(ECS::register_system(bt_teleport_bodies, "BtTeleportBodies")
								.execute_in(PHASE_CONFIG, "Physics")
								.set_description("Bullet Physics - Teleports the body on transform change, Handles the shape scaling."))

				.add(ECS::register_system(bt_update_rigidbody_transforms, "BtUpdateRigidBodyTransform")
								.execute_in(PHASE_CONFIG, "Physics")
								.set_description("Bullet Physics - Updates the transforms for the moved bodies."))

				.add(ECS::register_system(bt_config_box_shape, "BtConfigBoxShape")
								.execute_in(PHASE_CONFIG, "Physics")
								.set_description("Bullet Physics - Initialize and Updates the Box Shapes."))

				.add(ECS::register_system(bt_config_sphere_shape, "BtConfigSphereShape")
								.execute_in(PHASE_CONFIG, "Physics")
								.set_description("Bullet Physics - Initialize and Updates the Sphere Shapes."))

				.add(ECS::register_system(bt_config_capsule_shape, "BtConfigCapsuleShape")
								.execute_in(PHASE_CONFIG, "Physics")
								.set_description("Bullet Physics - Initialize and Updates the Capsule Shapes."))

				.add(ECS::register_system(bt_config_cone_shape, "BtConfigConeShape")
								.execute_in(PHASE_CONFIG, "Physics")
								.set_description("Bullet Physics - Initialize and Updates the Cone Shapes."))

				.add(ECS::register_system(bt_config_cylinder_shape, "BtConfigCylinderShape")
								.execute_in(PHASE_CONFIG, "Physics")
								.set_description("Bullet Physics - Initialize and Updates the Cylinder Shapes."))

				.add(ECS::register_system(bt_config_convex_shape, "BtConfigConvexShape")
								.execute_in(PHASE_CONFIG, "Physics")
								.set_description("Bullet Physics - Initialize and Updates the Convex Shapes."))

				.add(ECS::register_system(bt_config_trimesh_shape, "BtConfigTrimeshShape")
								.execute_in(PHASE_CONFIG, "Physics")
								.set_description("Bullet Physics - Initialize and Updates the Trimesh Shapes."))

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
								.set_description("Bullet Physics - Allow the areas to detect ovelapped bodies."));

		// Register Walk `System`s
		ECS::register_system(bt_pawn_walk, "BtPawnWalk")
				.execute_in(PHASE_PROCESS, "Physics")
				.set_description("Bullet Physics - Make the Rigidbody in kinematic mode walk according to Pawn settings.")
				.after("BtApplyForces")
				.before("BtSpacesStep");

		ECS::register_system_bundle("Bullet Physics All Features")
				.add("BtConfigBody")
				.add("BtConfigArea")
				.add("BtTeleportBodies")
				.add("BtUpdateRigidBodyTransform")
				.add("BtConfigBoxShape")
				.add("BtConfigSphereShape")
				.add("BtConfigCapsuleShape")
				.add("BtConfigConeShape")
				.add("BtConfigCylinderShape")
				.add("BtConfigConvexShape")
				.add("BtConfigTrimeshShape")
				.add("BtApplyForces")
				.add("BtPawnWalk")
				.add("BtSpacesStep")
				.add("BtOverlapCheck");

	} else if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
#ifdef TOOLS_ENABLED
		// Register gizmos
		Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtBoxGizmo));
		Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtSphereGizmo));
		Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtCapsuleGizmo));
		Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtConeGizmo));
		Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtCylinderGizmo));
		Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtConvexGizmo));
		Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtTrimeshGizmo));
		Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(BtPawnGizmo));
#endif
	}
}

void uninitialize_bullet_physics_module(ModuleInitializationLevel p_level) {
}
