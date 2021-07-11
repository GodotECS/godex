#include "register_types.h"

#include "components/disabled.h"
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

// TODO improve this workflow once the new pipeline is integrated.
class REP : public Object {
public:
	// TODO this should go in a better place, like at the end of the
	// engine setup: https://github.com/godotengine/godot-proposals/issues/1593
	void setup_ecs() {
		if (Engine::get_singleton()->is_editor_hint()) {
			if (EditorNode::get_singleton() != nullptr) {
				// Setup editor plugins
				EditorNode::get_singleton()->add_editor_plugin(memnew(EntityEditorPlugin(EditorNode::get_singleton())));
				EditorNode::get_singleton()->add_editor_plugin(memnew(WorldECSEditorPlugin(EditorNode::get_singleton())));
			}
			if (Node3DEditor::get_singleton() != nullptr) {
				// Add component gizmos:
				Node3DEditor::get_singleton()->add_gizmo_plugin(Ref<Components3DGizmoPlugin>(Components3DGizmoPlugin::get_singleton()));
			}
		} else {
			// Load the Scripted Components/Databags/Systems
			ScriptEcs::get_singleton()->register_runtime_scripts();
		}
	}
};

REP *rep = nullptr;

void register_godot_types() {
	rep = memnew(REP);

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Nodes
	ClassDB::register_class<ScriptEcs>();
	ClassDB::register_class<WorldECS>();
	ClassDB::register_class<PipelineECS>();
	ClassDB::register_class<Entity3D>();
	ClassDB::register_class<Entity2D>();
	ClassDB::register_class<Component>();
	ClassDB::register_class<System>();
	ClassDB::register_class<SystemBundle>();

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Gizmos
	Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(TransformComponentGizmo));
	Components3DGizmoPlugin::get_singleton()->add_component_gizmo(memnew(MeshComponentGizmo));

	// Register in editor things.
	if (MessageQueue::get_singleton()) {
		MessageQueue::get_singleton()->push_callable(callable_mp(rep, &REP::setup_ecs));
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Register engine components
	ECS::register_component<Child>([]() -> StorageBase * { return new Hierarchy; });
	ECS::register_component<Disabled>();
	ECS::register_component<MeshComponent>();
	ECS::register_component<TransformComponent>();
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
							.set_description("Throws events of finished event timers")
							.after("TimerUpdaterSystem"));

	ClassDB::register_class<SharedComponentResource>();

	memnew(ScriptEcs());
}

void unregister_godot_types() {
	memdelete(ScriptEcs::get_singleton());

	memdelete(rep);
	rep = nullptr;
}
