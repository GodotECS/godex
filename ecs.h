#pragma once

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
class EventStorageBase;

namespace godex {
class Databag;
class DynamicSystemExecutionData;
} // namespace godex

enum Phase : int {
	PHASE_MIN = 0,
	PHASE_CONFIG,
	PHASE_INPUT,
	PHASE_PRE_PROCESS,
	PHASE_PROCESS,
	PHASE_POST_PROCESS,
	PHASE_FINALIZE_PROCESS,
	PHASE_PRE_RENDER,
	PHASE_MAX,
};

struct DataAccessorFuncs {
	const LocalVector<PropertyInfo> *(*get_static_properties)() = nullptr;
	void (*get_property_list)(void *p_self, List<PropertyInfo> *r_list) = nullptr;
	Variant (*get_property_default)(const StringName &p_property_name) = nullptr;
	bool (*set_by_name)(void *p_self, const StringName &p_name, const Variant &p_data) = nullptr;
	bool (*get_by_name)(const void *p_self, const StringName &p_name, Variant &r_data) = nullptr;
	bool (*set_by_index)(void *p_self, const uint32_t p_parameter_index, const Variant &p_data) = nullptr;
	bool (*get_by_index)(const void *p_self, const uint32_t p_parameter_index, Variant &r_data) = nullptr;
	void (*call)(void *p_self, const StringName &p_method, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error) = nullptr;
};

struct SpawnerInfo {
	/// List of components spawner by this Spawner.
	LocalVector<godex::component_id> components;
};

/// These functions are implemented by the `COMPONENT` macro and assigned during
/// component registration.
struct ComponentInfo {
	StorageBase *(*create_storage)() = nullptr;
	void *(*new_component)() = nullptr;
	void (*free_component)(void *) = nullptr;
	void (*get_storage_config)(Dictionary &) = nullptr;
	DynamicComponentInfo *dynamic_component_info = nullptr;
	bool notify_release_write = false;
	bool is_shareable = false;
	LocalVector<godex::spawner_id> spawners;

	DataAccessorFuncs accessor_funcs;
};

/// These functions are implemented by the `DATABAG` macro and assigned during
/// component registration.
struct DatabagInfo {
	godex::Databag *(*create_databag)() = nullptr;

	DataAccessorFuncs accessor_funcs;
};

struct EventInfo {
	EventStorageBase *(*create_storage)() = nullptr;
	void (*destroy_storage)(EventStorageBase *p_storage) = nullptr;

	RBSet<String> event_emitters;
	bool event_emitters_need_reload = true;

	DataAccessorFuncs accessor_funcs;
};

enum Flags {
	NONE = 0,

	/// Do not insert inside the pipeline composition.
	EXCLUDE_PIPELINE_COMPOSITION = 1 << 1,
};

struct SystemDependency {
	bool execute_before;
	StringName system_name;
};

class SystemInfo {
	friend class ECS;
	friend class SystemBundleInfo;

	enum Type {
		TYPE_NORMAL,
		TYPE_DISPATCHER,
		TYPE_TEMPORARY,
		TYPE_DYNAMIC
	};

	godex::system_id id = godex::SYSTEM_NONE;
	Phase phase = PHASE_PROCESS;
	StringName dispatcher;
	LocalVector<SystemDependency> dependencies;
	String description;
	Type type = TYPE_NORMAL;
	int dispatcher_index = -1;
	int flags = Flags::NONE;

	// Only one of those is assigned (depending on the system type).
	func_get_system_exe_info exec_info = nullptr;
	func_temporary_system_execute temporary_exec = nullptr;

	func_system_data_get_size system_data_get_size = nullptr;
	func_system_data_new_placement system_data_new_placement = nullptr;
	func_system_data_delete_placement system_data_delete_placement = nullptr;
	func_system_data_set_active system_data_set_active = nullptr;

public:
	godex::system_id get_id() const;
	SystemInfo &execute_in(Phase p_phase, const StringName &p_dispatcher_name = StringName());
	SystemInfo &set_description(const String &p_description);
	SystemInfo &after(const StringName &p_system_name);
	SystemInfo &before(const StringName &p_system_name);
	SystemInfo &with_flags(int p_flags);
};

class SystemBundleInfo {
	friend class ECS;
	friend class PipelineBuilder;

	String description;
	LocalVector<StringName> systems;
	/// Bundle dependencies.
	LocalVector<SystemDependency> dependencies;

public:
	SystemBundleInfo &set_description(const String &p_description);

	SystemBundleInfo &after(const StringName &p_system_name);
	SystemBundleInfo &before(const StringName &p_system_name);
	SystemBundleInfo &add(const SystemInfo &p_system_info);
	SystemBundleInfo &add(const StringName &p_system_name);

private:
	void reset();
};

typedef void (*func_notify_static_destructor)();

class ECS : public Object {
	GDCLASS(ECS, Object)

	friend class Main;
	friend class godex::DynamicSystemExecutionData;

public:
	enum {
		NOTIFICATION_ECS_WORLD_LOADED = -1,
		NOTIFICATION_ECS_WORLD_PRE_UNLOAD = -2,
		NOTIFICATION_ECS_WORLD_UNLOADED = -3,
		NOTIFICATION_ECS_WORLD_READY = -4,
		NOTIFICATION_ECS_ENTITY_CREATED = -5
	};

private:
	static ECS *singleton;

	static int dispatcher_count;

	static LocalVector<StringName> spawners;
	static LocalVector<SpawnerInfo> spawners_info;

	static LocalVector<StringName> components;
	static LocalVector<ComponentInfo> components_info;

	static LocalVector<StringName> databags;
	static LocalVector<DatabagInfo> databags_info;

	static LocalVector<StringName> events;
	static LocalVector<EventInfo> events_info;

	static LocalVector<StringName> systems;
	static LocalVector<SystemInfo> systems_info;

	static LocalVector<StringName> system_bundles;
	static LocalVector<SystemBundleInfo> system_bundles_info;

	// Used to keep track of types that need static memory destruction.
	static LocalVector<func_notify_static_destructor> notify_static_destructor;

	// Node used by GDScript.
	WorldECS *active_world_node = nullptr;
	World *active_world = nullptr;
	bool ready = false;
	Pipeline *active_world_pipeline = nullptr;
	Token world_token;
	bool dispatching = false;

public:
	/// Clear the internal memory before the complete shutdown.
	static void __static_destructor();

	// ~~ Spawner ~~
	template <class I>
	static void register_spawner();

	static uint32_t get_spawners_count();
	static bool verify_spawner_id(godex::spawner_id p_spawner_id);

	static godex::spawner_id get_spawner_id(const StringName &p_name);
	static StringName get_spawner_name(godex::spawner_id p_spawner);

	static const LocalVector<godex::component_id> &get_spawnable_components(godex::spawner_id p_spawner);

	// ~~ Components ~~
	template <class C>
	static void register_component();

	template <class C>
	static void register_component(StorageBase *(*create_storage)());

	static uint32_t register_or_get_id_for_component_name(const StringName &p_name);
	static uint32_t register_or_update_script_component(const StringName &p_name, const LocalVector<ScriptProperty> &p_properties, StorageType p_storage_type, Vector<StringName> p_spawners);

	static uint32_t get_components_count();
	static bool verify_component_id(uint32_t p_component_id);

	static StorageBase *create_storage(godex::component_id p_component_id);
	static void *new_component(godex::component_id p_component_id);
	static void free_component(godex::component_id p_component_id, void *p_component);
	static void get_storage_config(godex::component_id p_component_id, Dictionary &r_config);
	static const LocalVector<StringName> &get_registered_components();
	static godex::component_id get_component_id(StringName p_component_name);
	static StringName get_component_name(godex::component_id p_component_id);
	static bool is_component_dynamic(godex::component_id p_component_id);
	static bool is_component_sharable(godex::component_id p_component_id);
	static bool storage_notify_release_write(godex::component_id p_component_id);

	static const LocalVector<PropertyInfo> *component_get_static_properties(godex::component_id p_component_id);
	static Variant get_component_property_default(godex::component_id p_component_id, StringName p_property_name);

	static const LocalVector<godex::spawner_id> &get_spawners(godex::component_id p_component_id);

	static bool unsafe_component_set_by_name(godex::component_id p_component_id, void *p_component, const StringName &p_name, const Variant &p_data);
	static bool unsafe_component_get_by_name(godex::component_id p_component_id, const void *p_component, const StringName &p_name, Variant &r_data);
	static Variant unsafe_component_get_by_name(godex::component_id p_component_id, const void *p_component, const StringName &p_name);
	static bool unsafe_component_set_by_index(godex::component_id p_component_id, void *p_component, uint32_t p_index, const Variant &p_data);
	static bool unsafe_component_get_by_index(godex::component_id p_component_id, const void *p_component, uint32_t p_index, Variant &r_data);
	static void unsafe_component_call(godex::component_id p_component_id, void *p_component, const StringName &p_method, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error);
	/// `p_component` can be `nullptr`, in that case only the static properties are returned.
	static void unsafe_component_get_property_list(godex::component_id p_component_id, void *p_component, List<PropertyInfo> *r_list);

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
	/// `p_databag` can be `nullptr`, in that case only the static properties are returned.
	static void unsafe_databag_get_property_list(godex::databag_id p_databag_id, void *p_databag, List<PropertyInfo> *r_list);

	// ~~ Events ~~
	template <class E>
	static void register_event();

	static bool verify_event_id(godex::event_id p_event_id);

	static godex::event_id get_event_id(const StringName &p_name);
	static StringName get_event_name(godex::event_id p_event_id);

	static EventStorageBase *create_events_storage(godex::event_id p_event_id);
	static void destroy_events_storage(godex::event_id p_event_id, EventStorageBase *p_storage);
	static const RBSet<String> &get_event_emitters(godex::event_id p_event_id);

	static bool unsafe_event_set_by_name(godex::event_id p_event_id, void *p_event, const StringName &p_name, const Variant &p_data);
	static bool unsafe_event_get_by_name(godex::event_id p_event_id, const void *p_event, const StringName &p_name, Variant &r_data);
	static Variant unsafe_event_get_by_name(godex::event_id p_event_id, const void *p_event, const StringName &p_name);
	static bool unsafe_event_set_by_index(godex::event_id p_event_id, void *p_event, uint32_t p_index, const Variant &p_data);
	static bool unsafe_event_get_by_index(godex::event_id p_event_id, const void *p_event, uint32_t p_index, Variant &r_data);
	static void unsafe_event_call(godex::event_id p_event_id, void *p_event, const StringName &p_method, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error);
	/// `p_event` can be `nullptr`, in that case only the static properties are returned.
	static void unsafe_event_get_property_list(godex::event_id p_event_id, void *p_event, List<PropertyInfo> *r_list);

	// ~~ SystemBundle ~~
	static SystemBundleInfo &register_system_bundle(const StringName &p_name);

	static uint32_t get_system_bundle_count();
	static godex::system_bundle_id get_system_bundle_id(const StringName &p_name);
	static StringName get_system_bundle_name(godex::system_bundle_id p_id);
	static const String &get_system_bundle_desc(godex::system_bundle_id p_id);
	static uint32_t get_system_bundle_systems_count(godex::system_bundle_id p_id);
	static SystemBundleInfo &get_system_bundle(godex::system_bundle_id p_id);

	// ~~ Systems ~~
	static SystemInfo &register_system(
			func_get_system_exe_info p_func_get_exe_info,
			func_system_data_get_size p_system_data_get_size,
			func_system_data_new_placement p_system_data_new_placement,
			func_system_data_delete_placement p_system_data_delete_placement,
			func_system_data_set_active p_system_data_set_active,
			StringName p_name);

// By defining the same name of the method, the IDE autocomplete shows the
// method name `register_system`, properly + it's impossible use the function
// directly by mistake.
#define register_system(func, name)                                                   \
	register_system(                                                                  \
			[](godex::system_id, SystemExeInfo &r_info) {                             \
				/* Take System Exe info. */                                           \
				SystemBuilder::get_system_info_from_function(r_info, func);           \
				r_info.system_func = [](uint8_t *p_mem, World *p_world) {             \
					SystemBuilder::system_exec_func(p_mem, p_world, func);            \
				};                                                                    \
			},                                                                        \
			[]() -> uint64_t {                                                        \
				/* Get size of SystemData */                                          \
				return SystemBuilder::system_data_size_of(func);                      \
			},                                                                        \
			[](uint8_t *p_mem, Token, World *p_world, Pipeline *, godex::system_id) { \
				/* SystemData New placement */                                        \
				SystemBuilder::system_data_new_placement(p_mem, p_world, func);       \
			},                                                                        \
			[](uint8_t *p_mem) {                                                      \
				/* SystemData Delete */                                               \
				SystemBuilder::system_data_delete_placement(p_mem, func);             \
			},                                                                        \
			[](uint8_t *p_mem, bool p_active) {                                       \
				/* SystemData set world notifications */                              \
				SystemBuilder::system_data_set_active(p_mem, p_active, func);         \
			},                                                                        \
			name)

	static void __process_pipeline_dispatcher(uint32_t p_count, Token p_token, Pipeline *p_pipeline, int p_dispatcher_index);

	static SystemInfo &register_system_dispatcher(
			func_get_system_exe_info p_func_get_exe_info,
			func_system_data_get_size p_system_data_get_size,
			func_system_data_new_placement p_system_data_new_placement,
			func_system_data_delete_placement p_system_data_delete_placement,
			func_system_data_set_active p_system_data_set_active,
			StringName p_name);

	// This macro defines all the SystemDispatcher functions.
#define register_system_dispatcher(func, name)                                                                        \
	register_system_dispatcher(                                                                                       \
			[](godex::system_id, SystemExeInfo &r_info) {                                                             \
				SystemBuilder::get_system_info_from_function(r_info, func);                                           \
				r_info.system_func = [](uint8_t *p_mem, World *p_world) {                                             \
					auto d = ((DispatcherSystemExecutionData *)p_mem);                                                \
					const uint32_t count = SystemBuilder::system_dispatcher_exec_func(                                \
							p_mem + sizeof(DispatcherSystemExecutionData),                                            \
							p_world,                                                                                  \
							func);                                                                                    \
					ECS::__process_pipeline_dispatcher(count, d->token, d->pipeline, d->dispatcher_index);            \
				};                                                                                                    \
			},                                                                                                        \
			[]() -> uint64_t {                                                                                        \
				/* Get size of SystemData */                                                                          \
				return sizeof(DispatcherSystemExecutionData) + SystemBuilder::system_data_size_of(func);              \
			},                                                                                                        \
			[](uint8_t *p_mem, Token p_token, World *p_world, Pipeline *p_pipeline, godex::system_id p_system_id) {   \
				/* SystemData New placement */                                                                        \
				/* First create the dispatcher data. */                                                               \
				DispatcherSystemExecutionData *d = new (p_mem) DispatcherSystemExecutionData;                         \
				d->token = p_token;                                                                                   \
				d->pipeline = p_pipeline;                                                                             \
				d->dispatcher_index = ECS::get_dispatcher_index(p_system_id);                                         \
				/* Now initialize the SystemExecutionData, like we do for a normal `System`.*/                        \
				SystemBuilder::system_data_new_placement(                                                             \
						p_mem + sizeof(DispatcherSystemExecutionData),                                                \
						p_world,                                                                                      \
						func);                                                                                        \
			},                                                                                                        \
			[](uint8_t *p_mem) {                                                                                      \
				/* SystemData Delete */                                                                               \
				((DispatcherSystemExecutionData *)p_mem)->~DispatcherSystemExecutionData();                           \
				SystemBuilder::system_data_delete_placement(p_mem + sizeof(DispatcherSystemExecutionData), func);     \
			},                                                                                                        \
			[](uint8_t *p_mem, bool p_active) {                                                                       \
				/* SystemData set world notifications */                                                              \
				SystemBuilder::system_data_set_active(p_mem + sizeof(DispatcherSystemExecutionData), p_active, func); \
			},                                                                                                        \
			name)

	/// Register the temporary system and returns the ID.
	static SystemInfo &register_temporary_system(
			func_temporary_system_execute p_func_temporary_systems_exe,
			func_system_data_get_size p_system_data_get_size,
			func_system_data_new_placement p_system_data_new_placement,
			func_system_data_delete_placement p_system_data_delete_placement,
			func_system_data_set_active p_system_data_set_active,
			StringName p_name);

	// By defining the same name of the method, the IDE autocomplete shows the
	// method name `register_temporary_system`, properly + it's impossible use the function
	// directly by mistake.
	// TODO use the same technique used for the system_dispatcher and normal system so we can retrieve the fetched data?
#define register_temporary_system(func, name)                                           \
	register_temporary_system(                                                          \
			[](uint8_t *p_mem, World *p_world) -> bool {                                \
				return SystemBuilder::temporary_system_exec_func(p_mem, p_world, func); \
			},                                                                          \
			[]() -> uint64_t {                                                          \
				/* Get size of SystemData */                                            \
				return SystemBuilder::system_data_size_of(func);                        \
			},                                                                          \
			[](uint8_t *p_mem, Token, World *p_world, Pipeline *, godex::system_id) {   \
				/* SystemData New placement */                                          \
				SystemBuilder::system_data_new_placement(p_mem, p_world, func);         \
			},                                                                          \
			[](uint8_t *p_mem) {                                                        \
				/* SystemData Delete */                                                 \
				SystemBuilder::system_data_delete_placement(p_mem, func);               \
			},                                                                          \
			[](uint8_t *p_mem, bool p_active) {                                         \
				/* SystemData set world notifications */                                \
				SystemBuilder::system_data_set_active(p_mem, p_active, func);           \
			},                                                                          \
			name)

	// Register the system and returns the ID.
	static SystemInfo &register_dynamic_system(StringName p_name);

	// This macro save the user the need to pass a `SystemExeInfo`, indeed it wraps
	// the passed function with a labda function that creates a `SystemExeInfo`.
	/// Returns the system id or UINT32_MAX if not found.
	static godex::system_id get_system_id(const StringName &p_name);
	static uint32_t get_systems_count();
	static bool can_systems_run_in_parallel(godex::system_id p_system_a, godex::system_id p_system_b);

private:
	static SystemInfo &get_system_info(godex::system_id p_id);

public:
	/// Returns the `SystemExeInfo`.
	static void get_system_exe_info(godex::system_id p_id, SystemExeInfo &r_info);
	static StringName get_system_name(godex::system_id p_id);
	static String get_system_desc(godex::system_id p_id);
	static Phase get_system_phase(godex::system_id p_id);
	static StringName get_system_dispatcher(godex::system_id p_id);
	static int get_dispatcher_index(godex::system_id p_id);
	static const LocalVector<SystemDependency> &get_system_dependencies(godex::system_id p_id);
	static int get_system_flags(godex::system_id p_id);

	/// Returns `true` when the system dispatches a pipeline when executed.
	static bool is_system_dispatcher(godex::system_id p_id);

	/// Returns `true` when the system is a temporary `System`.
	static bool is_temporary_system(godex::system_id p_id);
	static bool is_dynamic_system(godex::system_id p_id);

	static func_temporary_system_execute get_func_temporary_system_exe(godex::system_id p_id);

	static bool verify_system_id(godex::system_id p_id);

	static int get_dispatchers_count();

	/// Returns the size of `SystemExecutionData` used by this system.
	static uint64_t system_get_size_system_data(godex::system_id p_id);

	/// Used to create a `SystemExecutionData` in the givem pre-allocated memory address.
	/// `p_mem` must point to a preallocated memory, of the size given by
	/// `system_get_size_system_data`.
	static void system_new_placement_system_data(godex::system_id p_id, uint8_t *p_mem, Token p_token, World *p_world, Pipeline *p_pipeline);

	/// Delete the pointer allocated by `system_new_placement_system_data`.
	static void system_delete_placement_system_data(godex::system_id p_id, uint8_t *p_mem);

	/// Set this system Active or Unactive; the system is supposed to be processed
	/// only if Active.
	/// The Pipeline preparation and Pipeline activation are decoupled so it's
	/// possible to prepare a `World` to dispatch several pipelines, during load
	/// time, making the pipeline switch immediate.
	static void system_set_active_system(godex::system_id p_id, uint8_t *p_mem, bool p_active);

	static void preload_scripts();

private:
	static void clear_emitters_for_system(godex::system_id p_id);

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

	godex::component_id get_spawner_id_obj(StringName p_spawner_name) const {
		return get_spawner_id(p_spawner_name);
	}

	bool verify_spawner_id_obj(godex::spawner_id p_id) const {
		return verify_spawner_id(p_id);
	}

	godex::component_id get_component_id_obj(StringName p_component_name) const {
		return get_component_id(p_component_name);
	}

	bool verify_component_id_obj(godex::component_id p_id) const {
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

	bool is_dispatching() const {
		return dispatching;
	}

private:
	void dispatch_active_world();
	void ecs_init();
};

template <class I>
void ECS::register_spawner() {
	ERR_FAIL_COND_MSG(I::get_spawner_id() != UINT32_MAX, "This spawner is already registered.");

	const StringName name = I::get_class_static();
	I::spawner_id = spawners.size();

	spawners.push_back(name);
	spawners_info.push_back({});

	// Add a new scripting constant, for fast and easy `spawner` access.
	ClassDB::bind_integer_constant(get_class_static(), StringName(), name, I::spawner_id);

	print_line("Spawner: " + name + " registered with ID: " + itos(I::spawner_id));
}

template <class C>
void ECS::register_component() {
	register_component<C>(C::create_storage_no_type);
}

template <class C>
void ECS::register_component(StorageBase *(*create_storage)()) {
	bool notify_release_write = false;
	bool shared_component_storage = false;
	bool steady = false;
	{
		// This storage wants to be notified once the write object is released?
		StorageBase *s = create_storage();
		notify_release_write = s->notify_release_write();

		if (dynamic_cast<SharedStorage<C> *>(s) != nullptr) {
			// This is a shared component storage
			shared_component_storage = true;
		}

		steady = s->is_steady();

		delete s;
	}

	if (steady == false) {
		// The comopnent is using a storage that is not steady; it moves the
		// `Component` when it has to resize its internal memory.
		// So better to notify the user if the `Component` is not trivial.
		if (std::is_trivially_copyable<C>::value == false)
			WARN_PRINT("The component " + C::get_class_static() + " is not trivial copyable");
		if (std::is_trivially_destructible<C>::value == false)
			WARN_PRINT("The component " + C::get_class_static() + " is not trivial destructible");
		if (std::is_trivially_copy_assignable<C>::value == false)
			WARN_PRINT("The component " + C::get_class_static() + " is not trivial copy assignable");
		if (std::is_trivially_move_assignable<C>::value == false)
			WARN_PRINT("The component " + C::get_class_static() + " is not trivial move assignable");
		if (std::is_trivially_move_constructible<C>::value == false)
			WARN_PRINT("The component " + C::get_class_static() + " is not trivial move constructible");
	}

	ERR_FAIL_COND_MSG(C::get_component_id() != UINT32_MAX, "This component is already registered.");

	StringName component_name = C::get_class_static();
	C::component_id = components.size();

	if constexpr (godex_has__bind_methods<C>::value) {
		C::_bind_methods();
	}

	void (*get_storage_config)(Dictionary &) = nullptr;
	if constexpr (godex_has_storage_config<C>::value) {
		get_storage_config = C::_get_storage_config;
	}

	LocalVector<godex::spawner_id> spawners;
	if constexpr (godex_has_get_spawners<C>::value) {
		spawners = C::get_spawners();
		for (uint32_t i = 0; i < spawners.size(); i += 1) {
			spawners_info[spawners[i]].components.push_back(C::component_id);
		}
	}

	components.push_back(component_name);
	components_info.push_back(
			ComponentInfo{
					create_storage,
					C::new_component,
					C::free_component,
					get_storage_config,
					nullptr,
					notify_release_write,
					shared_component_storage,
					spawners,
					DataAccessorFuncs{
							C::get_static_properties,
							C::get_property_list,
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

template <class R>
void ECS::register_databag() {
	ERR_FAIL_COND_MSG(R::get_databag_id() != UINT32_MAX, "This databag is already registered.");

	StringName databag_name = R::get_class_static();
	R::databag_id = databags.size();
	if constexpr (godex_has__bind_methods<R>::value) {
		R::_bind_methods();
	}
	databags.push_back(databag_name);
	databags_info.push_back(DatabagInfo{
			R::create_databag_no_type,
			DataAccessorFuncs{
					R::get_static_properties,
					R::get_property_list,
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

	print_line("Databag: " + databag_name + " registered with ID: " + itos(R::databag_id));
}

template <class E>
void ECS::register_event() {
	StringName event_name = E::get_class_static();
	E::event_id = events.size();
	if constexpr (godex_has__bind_methods<E>::value) {
		E::_bind_methods();
	}
	events.push_back(event_name);
	events_info.push_back(EventInfo{
			E::create_storage_no_type,
			E::destroy_storage_no_type,
			RBSet<String>(),
			true,
			DataAccessorFuncs{
					E::get_static_properties,
					E::get_property_list,
					E::get_property_default,
					E::set_by_name,
					E::get_by_name,
					E::set_by_index,
					E::get_by_index,
					E::static_call } });

	// Store the function pointer that clear the static memory.
	notify_static_destructor.push_back(&E::__static_destructor);

	// Add a new scripting constant, for fast and easy `Event` access.
	ClassDB::bind_integer_constant(get_class_static(), StringName(), event_name, E::event_id);

	print_line("Event: " + event_name + " registered with ID: " + itos(E::event_id));
}

VARIANT_ENUM_CAST(Space)
VARIANT_ENUM_CAST(Phase)
