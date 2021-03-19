#include "register_types.h"

#include "components/disabled.h"
#include "components/mesh_component.h"
#include "components/physics/shape_3d_component.h"
#include "components/transform_component.h"
#include "core/object/message_queue.h"
#include "databags/godot_engine_databags.h"
#include "databags/input_databag.h"
#include "databags/visual_servers_databags.h"
#include "editor_plugins/components_gizmo_3d.h"
#include "editor_plugins/editor_world_ecs.h"
#include "editor_plugins/entity_editor_plugin.h"
#include "nodes/ecs_utilities.h"
#include "nodes/ecs_world.h"
#include "nodes/entity.h"
#include "nodes/shared_component_resource.h"
#include "systems/mesh_updater_system.h"
#include "systems/physics_process_system.h"

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
};

REP *rep = nullptr;

void ecs_register_godot_types() {
	rep = memnew(REP);

	ClassDB::register_class<WorldECS>();
	ClassDB::register_class<PipelineECS>();
	ClassDB::register_class<Entity3D>();
	ClassDB::register_class<Entity2D>();
	ClassDB::register_class<Component>();
	ClassDB::register_class<System>();

	// Register in editor things.
	if (MessageQueue::get_singleton()) {
		MessageQueue::get_singleton()->push_callable(callable_mp(rep, &REP::setup_ecs));
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Register engine components
	ECS::register_component<Child>([]() -> StorageBase * { return memnew(Hierarchy); });
	ECS::register_component<Disabled>();
	ECS::register_component<MeshComponent>();
	ECS::register_component<TransformComponent>();
	ECS::register_component<Shape3DComponent>();

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Register engine databags
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

	// Input
	ECS::register_databag<InputDatabag>();

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

	ClassDB::register_class<SharedComponentResource>();
}

void ecs_unregister_godot_types() {
	// Clear ScriptECS static memory.
	ScriptECS::__static_destructor();

	memdelete(rep);
	rep = nullptr;
}
