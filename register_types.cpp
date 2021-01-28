
#include "./register_types.h"

#include "core/config/engine.h"
#include "core/object/message_queue.h"
#include "ecs.h"
#include "editor/plugins/node_3d_editor_plugin.h"
#include "iterators/dynamic_query.h"
#include "systems/dynamic_system.h"

#include "godot/components/child.h"
#include "godot/components/disabled.h"
#include "godot/components/mesh_component.h"
#include "godot/components/physics/shape_3d_component.h"
#include "godot/components/transform_component.h"
#include "godot/databags/godot_engine_databags.h"
#include "godot/databags/visual_servers_databags.h"
#include "godot/editor_plugins/components_gizmo_3d.h"
#include "godot/editor_plugins/editor_world_ecs.h"
#include "godot/editor_plugins/entity_editor_plugin.h"
#include "godot/nodes/ecs_utilities.h"
#include "godot/nodes/ecs_world.h"
#include "godot/nodes/entity.h"
#include "godot/systems/mesh_updater_system.h"
#include "godot/systems/physics_process_system.h"

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
				Ref<Components3DGizmoPlugin> component_gizmo(memnew(Components3DGizmoPlugin));

				// Add component gizmos:
				component_gizmo->add_component_gizmo(memnew(TransformComponentGizmo));
				component_gizmo->add_component_gizmo(memnew(MeshComponentGizmo));

				Node3DEditor::get_singleton()->add_gizmo_plugin(component_gizmo);
			}
		} else {
			// Load the Scripted Components/Databags/Systems
			ScriptECS::register_runtime_scripts();
		}
	}
} rep;

void register_godex_types() {
	godex::DynamicSystemInfo::for_each_name = StringName("_for_each");

	ClassDB::register_class<ECS>();
	ClassDB::register_class<WorldECS>();
	ClassDB::register_class<PipelineECS>();
	ClassDB::register_class<Entity3D>();
	ClassDB::register_class<Entity2D>();
	ClassDB::register_class<Component>();
	ClassDB::register_class<System>();
	ClassDB::register_class<godex::DynamicQuery>();
	// One day this will be inverted.
	ClassDB::add_compatibility_class("Entity", "Entity3D");

	// Create and register singleton
	ECS *ecs = memnew(ECS);
	ECS::__set_singleton(ecs);
	Engine::get_singleton()->add_singleton(Engine::Singleton("ECS", ecs));

	// Register in editor things.
	if (MessageQueue::get_singleton()) {
		MessageQueue::get_singleton()->push_callable(callable_mp(&rep, &REP::setup_ecs));
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Register engine components
	ECS::register_component<Child>();
	ECS::register_component<Disabled>();
	ECS::register_component<MeshComponent>();
	ECS::register_component<TransformComponent>();
	ECS::register_component<Shape3DComponent>();

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Register engine databags
	ECS::register_databag<WorldCommands>();
	ECS::register_databag<World>();

	// Engine
	ECS::register_databag<FrameTime>();
	ECS::register_databag<OsDatabag>();
	ECS::register_databag<EngineDatabag>();
	ECS::register_databag<MessageQueueDatabag>();

	// Rendering
	ECS::register_databag<RenderingServerDatabag>();
	ECS::register_databag<RenderingScenarioDatabag>();

	// Physics
	ECS::register_databag<Physics3D>();

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Register engine systems
	// Engine
	ECS::register_system(call_physics_process, "CallPhysicsProcess", "Updates the Godot Nodes (2D/3D) transform and fetches the events from the physics engine.");

	// Rendering
	ECS::register_system(scenario_manager_system, "ScenarioManagerSystem", "Compatibility layer that allow to read the main window scenario and put in the ECS lifecycle; so that `MeshComponent` can properly show the mesh.");
	ECS::register_system(mesh_updater_system, "MeshUpdaterSystem", "Handles the mesh lifetime. This is required if you want to use `MeshComponent`");
	ECS::register_system(mesh_transform_updater_system, "MeshTransformUpdaterSystem", "Handles the mesh transformation. This is required if you want to use `MeshComponent`");

	// Physics 3D
	{
		const godex::system_id id = ECS::register_dynamic_system("PhysicsSystemDispatcher", "System that dispatches the specified pipeline at fixed rate. The rate is defined by `Physics Hz` in the project settings.");
		create_physics_system_dispatcher(ECS::get_dynamic_system_info(id));
	}
	ECS::register_system(step_physics_server_3d, "StepPhysicsServer3D", "Steps the PhysicsServer3D.");
}

void unregister_godex_types() {
	ECS *ecs = ECS::get_singleton();
	ECS::__set_singleton(nullptr);
	memdelete(ecs);
}
