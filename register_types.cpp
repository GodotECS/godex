
#include "./register_types.h"

#include "components/child.h"
#include "core/config/engine.h"
#include "databags/frame_time.h"
#include "ecs.h"
#include "editor/plugins/node_3d_editor_plugin.h"
#include "iterators/dynamic_query.h"
#include "modules/godot/editor_plugins/components_gizmo_3d.h"
#include "pipeline/pipeline_commands.h"
#include "systems/dynamic_system.h"
#include "utils/fetchers.h"

Ref<Components3DGizmoPlugin> component_gizmo;

void initialize_godex_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		ClassDB::register_class<ECS>();
		GDREGISTER_ABSTRACT_CLASS(GodexWorldFetcher);
		ClassDB::register_class<godex::DynamicQuery>();
		ClassDB::register_class<ComponentDynamicExposer>();
		ClassDB::register_class<DatabagDynamicFetcher>();
		ClassDB::register_class<StorageDynamicFetcher>();
		ClassDB::register_class<EventsEmitterDynamicFetcher>();
		ClassDB::register_class<EventsReceiverDynamicFetcher>();

		// One day this will be inverted.
		ClassDB::add_compatibility_class("Entity", "Entity3D");

		// Create and register singleton
		ECS *ecs = memnew(ECS);
		ECS::__set_singleton(ecs);
		Engine::get_singleton()->add_singleton(Engine::Singleton("ECS", ecs));

		ECS::register_databag<WorldCommands>();
		ECS::register_databag<World>();
		ECS::register_databag<PipelineCommands>();
		ECS::register_databag<FrameTime>();
	} else if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
#ifdef TOOLS_ENABLED
		component_gizmo.instantiate();
#endif
	}
}

void uninitialize_godex_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		// Clear ECS static memory.
		ECS::__static_destructor();
		ECS *ecs = ECS::get_singleton();
		ECS::__set_singleton(nullptr);
		memdelete(ecs);

		component_gizmo = Ref<Components3DGizmoPlugin>();
	}
}
