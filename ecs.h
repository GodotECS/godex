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

struct DataAccessorFuncs {
	const LocalVector<PropertyInfo> *(*get_properties)() = nullptr;
	Variant (*get_property_default)(const StringName &p_property_name) = nullptr;
	bool (*set_by_name)(void *p_self, const StringName &p_name, const Variant &p_data) = nullptr;
	bool (*get_by_name)(const void *p_self, const StringName &p_name, Variant &r_data) = nullptr;
	bool (*set_by_index)(void *p_self, const uint32_t p_parameter_index, const Variant &p_data) = nullptr;
	bool (*get_by_index)(const void *p_self, const uint32_t p_parameter_index, Variant &r_data) = nullptr;
	void (*call)(void *p_self, const StringName &p_method, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error) = nullptr;
};

/// These functions are implemented by the `COMPONENT` macro and assigned during
/// component registration.
struct ComponentInfo {
	StorageBase *(*create_storage)();
	DynamicComponentInfo *dynamic_component_info = nullptr;
	bool notify_release_write = false;
	bool is_event = false;

	DataAccessorFuncs accessor_funcs;
};

/// These functions are implemented by the `DATABAG` macro and assigned during
/// component registration.
struct DatabagInfo {
	godex::Databag *(*create_databag)();

	DataAccessorFuncs accessor_funcs;
};

struct SystemInfo {
	String description;

	// Only one of those is assigned (depending on the system type).
	uint32_t dynamic_system_id = UINT32_MAX;
	func_get_system_exe_info exec_info = nullptr;
	func_temporary_system_execute temporary_exec = nullptr;
};

typedef void (*func_notify_static_destructor)();

class ECS : public Object {
	GDCLASS(ECS, Object)

	friend class Main;

public:
	enum {
		NOTIFICATION_ECS_WORLD_LOADED = -1,
		NOTIFICATION_ECS_WORLD_PRE_UNLOAD = -2,
		NOTIFICATION_ECS_WORLD_UNLOADED = -3,
		NOTIFICATION_ECS_WORDL_READY = -4,
		NOTIFICATION_ECS_ENTITY_CREATED = -5
	};

private:
	static ECS *singleton;

	static LocalVector<StringName> components;
	static LocalVector<ComponentInfo> components_info;

	static LocalVector<StringName> databags;
	static LocalVector<DatabagInfo> databags_info;

	static LocalVector<StringName> systems;
	static LocalVector<SystemInfo> systems_info;

	// Used to keep track of types that need static memory destruction.
	static LocalVector<func_notify_static_destructor> notify_static_destructor;

	// Node used by GDScript.
	WorldECS *active_world_node = nullptr;
	World *active_world = nullptr;
	bool ready = false;
	Pipeline *active_world_pipeline = nullptr;
	bool dispatching = false;

public:
	/// Clear the internal memory before the complete shutdown.
	static void __static_destructor();

	// ~~ Components ~~
	template <class C>
	static void register_component();

	template <class C>
	static void register_component(StorageBase *(*create_storage)());

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
	static bool is_component_events(godex::component_id p_component_id);
	static bool storage_notify_release_write(godex::component_id p_component_id);

	static const LocalVector<PropertyInfo> *get_component_properties(godex::component_id p_component_id);
	static Variant get_component_property_default(godex::component_id p_component_id, StringName p_property_name);

	static bool unsafe_component_set_by_name(godex::component_id p_component_id, void *p_component, const StringName &p_name, const Variant &p_data);
	static bool unsafe_component_get_by_name(godex::component_id p_component_id, const void *p_component, const StringName &p_name, Variant &r_data);
	static Variant unsafe_component_get_by_name(godex::component_id p_component_id, const void *p_component, const StringName &p_name);
	static bool unsafe_component_set_by_index(godex::component_id p_component_id, void *p_component, uint32_t p_index, const Variant &p_data);
	static bool unsafe_component_get_by_index(godex::component_id p_component_id, const void *p_component, uint32_t p_index, Variant &r_data);
	static void unsafe_component_call(godex::component_id p_component_id, void *p_component, const StringName &p_method, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error);

	// ~~ Databags ~~
	template <class C>
	static void register_databag();

	static bool verify_databag_id(godex::databag_id p_id);

	static godex::Databag *create_databag(godex::databag_id p_id);
	static uint32_t get_databag_count();
	static godex::databag_id get_databag_id(const StringName &p_name);
	static StringName get_databag_name(godex::databag_id p_databag_id);

	static bool unsafe_databag_set_by_name(godex::databag_id p_databag_id, void *p_databag, const StringName &p_name, const Variant &p_data);
	static bool unsafe_databag_get_by_name(godex::databag_id p_databag_id, const void *p_databag, const StringName &p_name, Variant &r_data);
	static Variant unsafe_databag_get_by_name(godex::databag_id p_databag_id, const void *p_databag, const StringName &p_name);
	static bool unsafe_databag_set_by_index(godex::databag_id p_databag_id, void *p_databag, uint32_t p_index, const Variant &p_data);
	static bool unsafe_databag_get_by_index(godex::databag_id p_databag_id, const void *p_databag, uint32_t p_index, Variant &r_data);
	static void unsafe_databag_call(godex::databag_id p_databag_id, void *p_databag, const StringName &p_method, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error);

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
	bool is_world_ready() const;

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
	register_component<C>(C::create_storage_no_type);
}

template <class C>
void ECS::register_component(StorageBase *(*create_storage)()) {
	ERR_FAIL_COND_MSG(C::get_component_id() != UINT32_MAX, "This component is already registered.");

	bool notify_release_write = false;
	{
		// This storage wants to be notified once the write object is released?
		StorageBase *s = create_storage();
		notify_release_write = s->notify_release_write();
		memdelete(s);
	}

	StringName component_name = C::get_class_static();
	C::component_id = components.size();
	C::_bind_methods();
	components.push_back(component_name);
	components_info.push_back(
			ComponentInfo{
					create_storage,
					nullptr,
					notify_release_write,
					false,
					DataAccessorFuncs{
							C::get_properties,
							C::get_property_default,
							C::set_by_name,
							C::get_by_name,
							C::set_by_index,
							C::get_by_index,
							C::static_call } });

	// Store the function pointer that clear the static memory.
	notify_static_destructor.push_back(&C::__static_destructor);

	// Add a new scripting constant, for fast and easy `component` access.
	ClassDB::bind_integer_constant(get_class_static(), StringName(), component_name, C::component_id);

	print_line("Component: " + component_name + " registered with ID: " + itos(C::component_id));
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
			R::create_databag_no_type,
			DataAccessorFuncs{
					R::get_properties,
					R::get_property_default,
					R::set_by_name,
					R::get_by_name,
					R::set_by_index,
					R::get_by_index,
					R::static_call } });

	// Store the function pointer that clear the static memory.
	notify_static_destructor.push_back(&R::__static_destructor);

	// Add a new scripting constant, for fast and easy `databag` access.
	ClassDB::bind_integer_constant(get_class_static(), StringName(), databag_name, R::databag_id);
}

VARIANT_ENUM_CAST(Space);
