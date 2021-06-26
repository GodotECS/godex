
#include "./register_types.h"

#include "components/child.h"
#include "core/config/engine.h"
#include "databags/frame_time.h"
#include "ecs.h"
#include "editor/plugins/node_3d_editor_plugin.h"
#include "iterators/dynamic_query.h"
#include "modules/godot/editor_plugins/components_gizmo_3d.h"
#include "systems/dynamic_system.h"

Ref<Components3DGizmoPlugin> component_gizmo;

void register_godex_types() {
	component_gizmo.instantiate();

	godex::DynamicSystemInfo::for_each_name = StringName("_for_each");

	ClassDB::register_class<ECS>();
	ClassDB::register_class<godex::DynamicQuery>();

	// One day this will be inverted.
	ClassDB::add_compatibility_class("Entity", "Entity3D");

	// Create and register singleton
	ECS *ecs = memnew(ECS);
	ECS::__set_singleton(ecs);
	Engine::get_singleton()->add_singleton(Engine::Singleton("ECS", ecs));

	ECS::register_databag<WorldCommands>();
	ECS::register_databag<World>();
	ECS::register_databag<FrameTime>();
}

void unregister_godex_types() {
	// Clear dynamic system static memory.
	godex::__dynamic_system_info_static_destructor();
	godex::DynamicSystemInfo::for_each_name = StringName();

	// Clear ECS static memory.
	ECS::__static_destructor();
	ECS *ecs = ECS::get_singleton();
	ECS::__set_singleton(nullptr);
	memdelete(ecs);

	component_gizmo = Ref<Components3DGizmoPlugin>();
}
