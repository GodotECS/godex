#include "register_types.h"

#include "components/disabled.h"
#include "components/interpolated_transform_component.h"
#include "components/mesh_component.h"
#include "components/physics/shape_3d_component.h"
#include "components/transform_component.h"
#include "core/object/message_queue.h"
#include "databags/databag_timer.h"
#include "databags/godot_engine_databags.h"
#include "databags/input_databag.h"
#include "databags/scene_tree_databag.h"
#include "databags/visual_servers_databags.h"
#include "editor_plugins/components_mesh_gizmo_3d.h"
#include "editor_plugins/components_transform_gizmo_3d.h"
#include "editor_plugins/editor_world_ecs.h"
#include "editor_plugins/entity_editor_plugin.h"
#include "nodes/ecs_world.h"
#include "nodes/entity.h"
#include "nodes/script_ecs.h"
#include "nodes/shared_component_resource.h"
#include "systems/mesh_updater_system.h"
#include "systems/physics_process_system.h"
#include "systems/timer_updater_system.h"

#include "editor/plugins/node_3d_editor_plugin.h"

extern Ref<Components3DGizmoPlugin> component_gizmo;

static void _editor_init() {
	EditorNode *p_editor = EditorNode::get_singleton();
	ERR_FAIL_COND_MSG(p_editor == nullptr, "The editor is not defined.");

	EntityEditorPlugin *entity_plugin = memnew(EntityEditorPlugin(p_editor));
	EditorNode::get_singleton()->add_editor_plugin(entity_plugin);

	WorldECSEditorPlugin *worldecs_plugin = memnew(WorldECSEditorPlugin(p_editor));
	EditorNode::get_singleton()->add_editor_plugin(worldecs_plugin);

	Node3DEditor::get_singleton()->add_gizmo_plugin(Ref<Components3DGizmoPlugin>(Components3DGizmoPlugin::get_singleton()));
	component_gizmo = Ref<Components3DGizmoPlugin>();
}

void initialize_godot_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Nodes
		ClassDB::register_class<ScriptEcs>();
		ClassDB::register_class<WorldECS>();
		ClassDB::register_class<PipelineECS>();
		ClassDB::register_class<Entity3D>();
		ClassDB::register_class<Entity2D>();
		ClassDB::register_class<Component>();
		ClassDB::register_class<System>();
		ClassDB::register_class<SystemBundle>();

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Register engine components
		ECS::register_component<Child>([]() -> StorageBase * { return new Hierarchy; });
		ECS::register_component<Disabled>();
		ECS::register_component<MeshComponent>();
		ECS::register_component<TransformComponent>();
		ECS::register_component<InterpolatedTransformComponent>();
		ECS::register_component<Shape3DComponent>();

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Register engine databags
		// Engine
		ECS::register_databag<SceneTreeDatabag>();
		ECS::register_databag<SceneTreeInfoDatabag>();
		ECS::register_databag<OsDatabag>();
		ECS::register_databag<EngineDatabag>();
		ECS::register_databag<MessageQueueDatabag>();

		// Rendering
		ECS::register_databag<RenderingServerDatabag>();
		ECS::register_databag<RenderingScenarioDatabag>();

		// Physics
		ECS::register_databag<Physics3D>();

		// Input
		ECS::register_databag<InputDatabag>();

		// Events
		ECS::register_databag<TimersDatabag>();

		// Engine

		// Rendering
		ECS::register_system_bundle("Rendering 3D")
				.set_description("3D Rendering mechanism.")
				.add(ECS::register_system(scenario_manager_system, "ScenarioManagerSystem")
								.execute_in(PHASE_CONFIG)
								.set_description("Compatibility layer that allow to read the main window scenario and put in the ECS lifecycle; so that `MeshComponent` can properly show the mesh."))

				.add(ECS::register_system(interpolates_transform, "InterpolatesTransform")
								.execute_in(PHASE_PRE_RENDER)
								.set_description("Interpolates the transform."))

				.add(ECS::register_system(mesh_updater_system, "MeshUpdaterSystem")
								.execute_in(PHASE_PRE_RENDER)
								.set_description("Updates the VisualServer with mesh component data"))

				.add(ECS::register_system(mesh_transform_updater_system, "MeshTransformUpdaterSystem")
								.execute_in(PHASE_PRE_RENDER)
								.set_description("Updates the VisualServer mesh transforms."));

		// Physics 3D
		ECS::register_system_bundle("Physics")
				.set_description(TTR("Physics mechanism."))
				.add(ECS::register_system_dispatcher(physics_pipeline_dispatcher, "Physics")
								.execute_in(PHASE_PROCESS)
								.set_description("Physics dispatcher"))

				.add(ECS::register_system(physics_init_frame, "physics_init_frame")
								.execute_in(PHASE_MIN, "Physics"))

				.add(ECS::register_system(call_physics_process, "CallPhysicsProcess")
								.execute_in(PHASE_PROCESS, "Physics")
								.set_description("Updates the Godot Nodes (2D/3D) transform and fetches the events from the physics engine."))

				.add(ECS::register_system(step_physics_server_3d, "StepPhysicsServer3D")
								.execute_in(PHASE_POST_PROCESS, "Physics")
								.set_description("Steps the PhysicsServer3D."))

				.add(ECS::register_system(physics_finalize_frame, "physics_finalize_frame")
								.execute_in(PHASE_MAX, "Physics"));

		// Utilities
		ECS::register_system_bundle("Utilities")
				.set_description("Core and global systems: timers, ... (Currently nothing else)")
				.add(ECS::register_system(timer_updater_system, "TimerUpdaterSystem")
								.execute_in(PHASE_CONFIG)
								.set_description("Updates the `TimerDatabag`"))

				.add(ECS::register_system(timer_event_launcher_system, "TimerEventLauncherSystem")
								.execute_in(PHASE_CONFIG)
								.set_description("Throws events of finished event timers"));

		ClassDB::register_class<SharedComponentResource>();

#ifdef DEBUG_ENABLED
		// TS is nullptr at this point when running tests in debug mode, and it causes a crash. I did not find a better way to check for that.
		if (TS != nullptr) {
			ECS::preload_scripts();
		}
#else
		ECS::preload_scripts();
#endif
		memnew(ScriptEcs());

	} else if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		if (Engine::get_singleton()->is_editor_hint()) {
			EditorNode::add_init_callback(_editor_init);

			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Gizmos
			Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(TransformComponentGizmo));
			Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(MeshComponentGizmo));

		} else {
			// Load the Scripted Components/Databags/Systems
			// ScriptEcs::get_singleton()->register_runtime_scripts();
		}
	}
}

void uninitialize_godot_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		if (!Engine::get_singleton()->is_editor_hint()) {
			ScriptEcs::get_singleton()->reset_editor_default_component_properties();
		}
		memdelete(ScriptEcs::get_singleton());
	} else if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		if (Engine::get_singleton()->is_editor_hint()) {
			ScriptEcs::get_singleton()->reset_editor_default_component_properties();
		}
	}
}
