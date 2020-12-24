
#include "./register_types.h"

#include "components/mesh_component.h"
#include "components/transform_component.h"
#include "core/config/engine.h"
#include "core/object/message_queue.h"
#include "ecs.h"
#include "nodes/ecs_utilities.h"
#include "nodes/ecs_world.h"
#include "nodes/entity.h"
#include "systems/dynamic_system.h"

#include "editor_plugins/editor_world_ecs.h"
#include "editor_plugins/entity_editor_plugin.h"

#include "resources/godot_engine_resources.h"

#include "systems/physics_process_system.h"

// TODO improve this workflow once the new pipeline is integrated.
class REP : public Object {
public:
	void register_editor_plugins() {
		if (EditorNode::get_singleton() != nullptr) {
			EditorNode::get_singleton()->add_editor_plugin(memnew(EntityEditorPlugin(EditorNode::get_singleton())));
			EditorNode::get_singleton()->add_editor_plugin(memnew(WorldECSEditorPlugin(EditorNode::get_singleton())));
		}
	}
} rep;

void register_godex_types() {
	godex::DynamicSystemInfo::for_each_name = StringName("_for_each");

	ClassDB::register_class<ECS>();
	ClassDB::register_class<WorldECS>();
	ClassDB::register_class<PipelineECS>();
	ClassDB::register_class<Entity>();
	ClassDB::register_class<Component>();
	ClassDB::register_class<System>();

	// Create and register singleton
	ECS *ecs = memnew(ECS);
	ECS::__set_singleton(ecs);
	Engine::get_singleton()->add_singleton(Engine::Singleton("ECS", ecs));

	// Register editor plugins
	if (Engine::get_singleton()->is_editor_hint()) {
		MessageQueue::get_singleton()->push_callable(callable_mp(&rep, &REP::register_editor_plugins));
	}

	// ~ Register engine components ~
	ECS::register_component<MeshComponent>();
	ECS::register_component<TransformComponent>();

	// ~ Register engine resources ~
	ECS::register_resource<GodotIteratorInfoResource>();
	ECS::register_resource<OsResource>();
	ECS::register_resource<EngineResource>();
	ECS::register_resource<Physics3DServerResource>();
	ECS::register_resource<MessageQueueResource>();

	// ~ Register engine systems ~
	//ECS::register_system(physics_2d_process_system, "Physics2dProcessSystem", "Steps the physics 2D [not yet implemented].");
	ECS::register_system(physics_3d_process_system, "Physics3dProcessSystem", "Steps the physics.");
}

void unregister_godex_types() {
	ECS *ecs = ECS::get_singleton();
	ECS::__set_singleton(nullptr);
	memdelete(ecs);
}
