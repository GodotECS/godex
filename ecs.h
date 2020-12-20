#pragma once

/* Author: AndreaCatania */

#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/templates/local_vector.h"
#include "core/templates/oa_hash_map.h"
#include "modules/ecs/world/world_commands.h"

#include "modules/ecs/systems/system.h"
#include "modules/ecs/systems/system_builder.h"

class World;
class Pipeline;
class DynamicComponentInfo;
namespace godex {

class DynamicSystemInfo;

typedef uint32_t component_id;
typedef uint32_t resource_id;
typedef uint32_t system_id;
} // namespace godex

// These functions are implemented by the `COMPONENT` macro and assigned during
// component registration.
struct ComponentInfo {
	LocalVector<PropertyInfo> *(*get_properties)();
	Variant (*get_property_default)(StringName p_property_name);
	Storage *(*create_storage)();
	DynamicComponentInfo *dynamic_component_info = nullptr;
};

class ECS : public Object {
	GDCLASS(ECS, Object)

	friend class Main;

	static ECS *singleton;
	static LocalVector<StringName> components;
	static LocalVector<ComponentInfo> components_info;

	static LocalVector<StringName> resources;

	static LocalVector<StringName> systems;
	static LocalVector<SystemInfo> systems_info;

	World *active_world = nullptr;
	Pipeline *active_world_pipeline = nullptr;
	WorldCommands commands;

public:
	// ~~ Components ~~
	template <class C>
	static void register_component();

	// TODO specify the storage here?
	static uint32_t register_script_component(StringName p_name, const LocalVector<ScriptProperty> &p_properties, StorageType p_storage_type);

	static bool verify_component_id(uint32_t p_component_id);

	static Storage *create_storage(godex::component_id p_component_id);
	static const LocalVector<StringName> &get_registered_components();
	static godex::component_id get_component_id(StringName p_component_name);
	static StringName get_component_name(godex::component_id p_component_id);
	static const LocalVector<PropertyInfo> *get_component_properties(godex::component_id p_component_id);
	static Variant get_component_property_default(godex::component_id p_component_id, StringName p_property_name);

	// ~~ Resources ~~
	template <class C>
	static void register_resource();

	static const LocalVector<StringName> &get_registered_resources();
	static godex::resource_id get_resource_id(const StringName &p_name);
	static StringName get_resource_name(godex::resource_id p_resource_id);

	// ~~ Systems ~~
	static void register_system(get_system_info_func p_get_info_func, StringName p_name, String p_description = "");

	// Register the system and returns the ID.
	static godex::system_id register_dynamic_system(StringName p_name, const godex::DynamicSystemInfo *p_info);

// This macro save the user the need to pass a `SystemInfo`, indeed it wraps
// the passed function with a labda function that creates a `SystemInfo`.
// By defining the same name of the method, the IDE autocomplete shows the method
// name `register_system`, properly + it's impossible use the function directly
// by mistake.
#define register_system(func, name, desc)                                  \
	register_system([]() -> SystemInfo {                                   \
		SystemInfo i = SystemBuilder::get_system_info_from_function(func); \
		i.system_func = [](World *p_world) {                               \
			SystemBuilder::system_exec_func(p_world, func);                \
		};                                                                 \
		return i;                                                          \
	},                                                                     \
			name, desc)

	/// Returns the system id or UINT32_MAX if not found.
	static godex::system_id find_system_id(StringName p_name);
	static uint32_t get_systems_count();
	static const SystemInfo &get_system_info(godex::system_id p_id);
	static void set_dynamic_system_target(godex::system_id p_id, Object *p_target);
	static bool verify_system_id(godex::system_id p_id);

protected:
	static void _bind_methods();

public:
	static void __set_singleton(ECS *p_singleton);
	static ECS *get_singleton();

public:
	ECS();

	virtual ~ECS();

	/// Set the active world. If there is already an active world an error
	/// is generated.
	void set_active_world(World *p_world);
	World *get_active_world() const;

	bool has_active_world() const;

	/// Returns a command object that can be used to spawn entities, add
	/// components.
	/// This function returns nullptr when the world is dispatched because
	/// it's unsafe interact during that phase.
	WorldCommands *get_commands();

	void set_active_world_pipeline(Pipeline *p_pipeline);
	Pipeline *get_active_world_pipeline() const;

	bool has_active_world_pipeline() const;

private:
	bool dispatch_active_world();
	void ecs_init();
};

template <class C>
void ECS::register_component() {
	ERR_FAIL_COND_MSG(C::get_component_id() != UINT32_MAX, "This component is already registered.");

	StringName component_name = C::get_class_static();
	C::component_id = components.size();
	C::_bind_properties();
	components.push_back(component_name);
	components_info.push_back(
			ComponentInfo{
					&C::get_properties_static,
					&C::get_property_default_static,
					&C::create_storage_no_type,
					nullptr });
}

template <class R>
void ECS::register_resource() {
	ERR_FAIL_COND_MSG(R::get_resource_id() != UINT32_MAX, "This resource is already registered.");

	StringName resource_name = R::get_class_static();
	R::resource_id = resources.size();
	R::_bind_properties();
	resources.push_back(resource_name);
}
