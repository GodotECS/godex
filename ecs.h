#pragma once

/* Author: AndreaCatania */

#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/templates/local_vector.h"
#include "core/templates/oa_hash_map.h"

#include "systems/system.h"
#include "systems/system_builder.h"

class World;
class WorldECS;
class Pipeline;
class DynamicComponentInfo;

namespace godex {
class Databag;
class DynamicSystemInfo;
} // namespace godex

/// These functions are implemented by the `COMPONENT` macro and assigned during
/// component registration.
struct ComponentInfo {
	LocalVector<PropertyInfo> *(*get_properties)();
	Variant (*get_property_default)(StringName p_property_name);
	StorageBase *(*create_storage)();
	DynamicComponentInfo *dynamic_component_info = nullptr;
	bool is_event = false;
};

/// These functions are implemented by the `DATABAG` macro and assigned during
/// component registration.
struct DatabagInfo {
	godex::Databag *(*create_databag)();
};

struct SystemInfo {
	String description;

	// Only one of those is assigned (depending on the system type).
	uint32_t dynamic_system_id = UINT32_MAX;
	func_get_system_exe_info exec_info = nullptr;
	func_temporary_system_execute temporary_exec = nullptr;
};

class ECS : public Object {
	GDCLASS(ECS, Object)

	friend class Main;

	static ECS *singleton;
	static LocalVector<StringName> components;
	static LocalVector<ComponentInfo> components_info;

	static LocalVector<StringName> databags;
	static LocalVector<DatabagInfo> databags_info;

	static LocalVector<StringName> systems;
	static LocalVector<SystemInfo> systems_info;

	// Node used by GDScript.
	WorldECS *active_world_node = nullptr;
	World *active_world = nullptr;
	Pipeline *active_world_pipeline = nullptr;
	bool dispatching = false;

public:
	// ~~ Components ~~
	template <class C>
	static void register_component();

	template <class E>
	static void register_component_event();

	// TODO specify the storage here?
	static uint32_t register_script_component(const StringName &p_name, const LocalVector<ScriptProperty> &p_properties, StorageType p_storage_type);
	static uint32_t register_script_component_event(const StringName &p_name, const LocalVector<ScriptProperty> &p_properties, StorageType p_storage_type);

	static bool verify_component_id(uint32_t p_component_id);

	static StorageBase *create_storage(godex::component_id p_component_id);
	static const LocalVector<StringName> &get_registered_components();
	static godex::component_id get_component_id(StringName p_component_name);
	static StringName get_component_name(godex::component_id p_component_id);
	static const LocalVector<PropertyInfo> *get_component_properties(godex::component_id p_component_id);
	static Variant get_component_property_default(godex::component_id p_component_id, StringName p_property_name);
	static bool is_component_events(godex::component_id p_component_id);

	// ~~ Databags ~~
	template <class C>
	static void register_databag();

	static bool verify_databag_id(godex::databag_id p_id);

	static godex::Databag *create_databag(godex::databag_id p_id);
	static uint32_t get_databag_count();
	static godex::databag_id get_databag_id(const StringName &p_name);
	static StringName get_databag_name(godex::databag_id p_databag_id);

	// ~~ Systems ~~
	static void register_system(func_get_system_exe_info p_func_get_exe_info, StringName p_name, String p_description = "");

// By defining the same name of the method, the IDE autocomplete shows the
// method name `register_system`, properly + it's impossible use the function
// directly by mistake.
#define register_system(func, name, desc)                           \
	register_system([](SystemExeInfo &r_info) {                     \
		SystemBuilder::get_system_info_from_function(r_info, func); \
		r_info.system_func = [](World *p_world) {                   \
			SystemBuilder::system_exec_func(p_world, func);         \
		};                                                          \
	},                                                              \
			name, desc)

	// Register the system and returns the ID.
	static godex::system_id register_dynamic_system(StringName p_name, const String &p_description = String());

	// This macro save the user the need to pass a `SystemExeInfo`, indeed it wraps
	// the passed function with a labda function that creates a `SystemExeInfo`.
	/// Returns the system id or UINT32_MAX if not found.
	static godex::system_id get_system_id(const StringName &p_name);
	static uint32_t get_systems_count();

	/// Returns the function that can be used to obtain the `SystemExeInfo`.
	static func_get_system_exe_info get_func_system_exe_info(godex::system_id p_id);

	/// Returns the `SystemExeInfo`.
	static void get_system_exe_info(godex::system_id p_id, SystemExeInfo &r_info);
	static StringName get_system_name(godex::system_id p_id);
	static String get_system_desc(godex::system_id p_id);

	static void set_dynamic_system_target(godex::system_id p_id, ScriptInstance *p_target);
	static godex::DynamicSystemInfo *get_dynamic_system_info(godex::system_id p_id);

	/// Returns `true` when the system dispatches a pipeline when executed.
	static bool is_system_dispatcher(godex::system_id p_id);
	/// Set the `SystemDispatcher` pipeline, does nothing if this system is not
	/// a `SystemDispatcher`.
	static void set_system_pipeline(godex::system_id p_id, Pipeline *p_pipeline);

	/// Register the temporary system and returns the ID.
	static void register_temporary_system(func_temporary_system_execute p_func_temporary_systems_exe, StringName p_name, const String &p_description = String());

// By defining the same name of the method, the IDE autocomplete shows the
// method name `register_temporary_system`, properly + it's impossible use the function
// directly by mistake.
#define register_temporary_system(func, name, desc)                      \
	register_temporary_system([](World *p_world) -> bool {               \
		return SystemBuilder::temporary_system_exec_func(p_world, func); \
	},                                                                   \
			name, desc)

	/// Returns `true` when the system is a temporary `System`.
	static bool is_temporary_system(godex::system_id p_id);

	static func_temporary_system_execute get_func_temporary_system_exe(godex::system_id p_id);

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
	void set_active_world(World *p_world, WorldECS *p_active_world_ecs);
	World *get_active_world();
	const World *get_active_world() const;
	Node *get_active_world_node();

	bool has_active_world() const;

	void set_active_world_pipeline(Pipeline *p_pipeline);
	Pipeline *get_active_world_pipeline() const;

	bool has_active_world_pipeline() const;

	godex::component_id get_component_id_obj(StringName p_component_name) const {
		return get_component_id(p_component_name);
	}

	bool verify_component_id_obj(godex::system_id p_id) const {
		return verify_component_id(p_id);
	}

	godex::databag_id get_databag_id_obj(StringName p_databag_name) const {
		return get_databag_id(p_databag_name);
	}

	bool verify_databag_id_obj(godex::databag_id p_id) const {
		return verify_databag_id(p_id);
	}

	godex::system_id get_system_id_obj(StringName p_system_name) const {
		return get_system_id(p_system_name);
	}

	bool verify_system_id_obj(godex::system_id p_id) const {
		return verify_system_id(p_id);
	}

private:
	void dispatch_active_world();
	void ecs_init();
};

template <class C>
void ECS::register_component() {
	ERR_FAIL_COND_MSG(C::get_component_id() != UINT32_MAX, "This component is already registered.");

	StringName component_name = C::get_class_static();
	C::component_id = components.size();
	C::_bind_methods();
	components.push_back(component_name);
	components_info.push_back(
			ComponentInfo{
					&C::get_properties_static,
					&C::get_property_default_static,
					&C::create_storage_no_type,
					nullptr });
}

template <class E>
void ECS::register_component_event() {
	ERR_FAIL_COND_MSG(E::get_component_id() != UINT32_MAX, "This component event is already registered.");
	register_component<E>();

#ifdef DEBUG_ENABLED
	// `register_component` is not supposed to fail.
	CRASH_COND(E::get_component_id() == UINT32_MAX);
#endif

	components_info[E::get_component_id()].is_event = true;
}

template <class R>
void ECS::register_databag() {
	ERR_FAIL_COND_MSG(R::get_databag_id() != UINT32_MAX, "This databag is already registered.");

	StringName databag_name = R::get_class_static();
	R::databag_id = databags.size();
	R::_bind_methods();
	databags.push_back(databag_name);
	databags_info.push_back(DatabagInfo{
			R::create_databag_no_type });
}
