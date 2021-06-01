
#include "ecs.h"

#include "components/dynamic_component.h"
#include "core/object/message_queue.h"
#include "modules/godot/nodes/ecs_world.h"
#include "pipeline/pipeline.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "systems/dynamic_system.h"
#include "world/world.h"

ECS *ECS::singleton = nullptr;
LocalVector<func_notify_static_destructor> ECS::notify_static_destructor;
LocalVector<StringName> ECS::spawners;
LocalVector<SpawnerInfo> ECS::spawners_info;
LocalVector<StringName> ECS::components;
LocalVector<ComponentInfo> ECS::components_info;
LocalVector<StringName> ECS::databags;
LocalVector<DatabagInfo> ECS::databags_info;
LocalVector<StringName> ECS::systems;
LocalVector<SystemInfo> ECS::systems_info;
LocalVector<StringName> ECS::system_bundles;
LocalVector<SystemBundleInfo> ECS::system_bundles_info;

godex::system_id SystemInfo::get_id() const {
	return id;
}

SystemInfo &SystemInfo::set_phase(Phase p_phase) {
	phase = p_phase;
	return *this;
}

SystemInfo &SystemInfo::set_description(const String &p_description) {
	description = p_description;
	return *this;
}

SystemInfo &SystemInfo::after(const StringName &p_system_name) {
	dependencies.push_back({ false, p_system_name });
	return *this;
}

SystemInfo &SystemInfo::before(const StringName &p_system_name) {
	dependencies.push_back({ true, p_system_name });
	return *this;
}

SystemBundleInfo &SystemBundleInfo::set_description(const String &p_description) {
	description = p_description;
	return *this;
}

SystemBundleInfo &SystemBundleInfo::after(const StringName &p_system_name) {
	dependencies.push_back({ false, p_system_name });
	return *this;
}

SystemBundleInfo &SystemBundleInfo::before(const StringName &p_system_name) {
	dependencies.push_back({ true, p_system_name });
	return *this;
}

SystemBundleInfo &SystemBundleInfo::add(const SystemInfo &p_system_info) {
	systems.push_back(ECS::get_system_name(p_system_info.id));
	return *this;
}

SystemBundleInfo &SystemBundleInfo::add(const StringName &p_system_name) {
	systems.push_back(p_system_name);
	return *this;
}

void SystemBundleInfo::reset() {
	description = String();
	systems.reset();
	dependencies.reset();
}

void ECS::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_active_world"), &ECS::get_active_world_node);

	ClassDB::bind_method(D_METHOD("get_spawner_id", "name"), &ECS::get_spawner_id_obj);
	ClassDB::bind_method(D_METHOD("verify_spawner_id", "id"), &ECS::verify_spawner_id_obj);

	ClassDB::bind_method(D_METHOD("get_component_id", "name"), &ECS::get_component_id_obj);
	ClassDB::bind_method(D_METHOD("verify_component_id", "id"), &ECS::verify_component_id_obj);

	ClassDB::bind_method(D_METHOD("get_databag_id", "name"), &ECS::get_databag_id_obj);
	ClassDB::bind_method(D_METHOD("verify_databag_id", "id"), &ECS::verify_databag_id_obj);

	ClassDB::bind_method(D_METHOD("get_system_id", "name"), &ECS::get_system_id_obj);
	ClassDB::bind_method(D_METHOD("verify_system_id", "id"), &ECS::verify_system_id_obj);

	BIND_CONSTANT(NOTIFICATION_ECS_WORLD_LOADED)
	BIND_CONSTANT(NOTIFICATION_ECS_WORLD_PRE_UNLOAD)
	BIND_CONSTANT(NOTIFICATION_ECS_WORLD_UNLOADED)
	BIND_CONSTANT(NOTIFICATION_ECS_WORLD_READY)
	BIND_CONSTANT(NOTIFICATION_ECS_ENTITY_CREATED)

	BIND_ENUM_CONSTANT(LOCAL);
	BIND_ENUM_CONSTANT(GLOBAL);

	BIND_ENUM_CONSTANT(PHASE_CONFIG);
	BIND_ENUM_CONSTANT(PHASE_INPUT);
	BIND_ENUM_CONSTANT(PHASE_PRE_PROCESS);
	BIND_ENUM_CONSTANT(PHASE_PROCESS);
	BIND_ENUM_CONSTANT(PHASE_POST_PROCESS);
}

void ECS::__static_destructor() {
	// Call static destructors.
	for (uint32_t i = 0; i < notify_static_destructor.size(); i += 1) {
		notify_static_destructor[i]();
	}

	spawners.reset();
	spawners_info.reset();

	// Clear the components static data.
	components.reset();
	components_info.reset();

	// Clear the databags static data.
	databags.reset();
	databags_info.reset();

	// Clear the system bundles static data.
	system_bundles.reset();
	system_bundles_info.reset();

	// Clear the systems static data.
	systems.reset();
	systems_info.reset();
}

ECS::ECS() :
		Object() {
	if (MessageQueue::get_singleton() != nullptr) {
		// TODO Do I need this? https://github.com/godotengine/godot-proposals/issues/1593
		MessageQueue::get_singleton()->push_callable(callable_mp(this, &ECS::ecs_init));
	}
}

ECS::~ECS() {
}

const LocalVector<StringName> &ECS::get_registered_components() {
	return components;
}

uint32_t ECS::get_component_id(StringName p_component_name) {
	const int64_t i = components.find(p_component_name);
	return i < 0 ? UINT32_MAX : uint32_t(i);
}

StringName ECS::get_component_name(uint32_t p_component_id) {
	ERR_FAIL_COND_V_MSG(verify_component_id(p_component_id) == false, StringName(), "The `component_id` is invalid: " + itos(p_component_id));
	return components[p_component_id];
}

bool ECS::is_component_dynamic(godex::component_id p_component_id) {
	ERR_FAIL_COND_V_MSG(verify_component_id(p_component_id) == false, false, "The component " + itos(p_component_id) + " is invalid.");
	return components_info[p_component_id].dynamic_component_info != nullptr;
}

bool ECS::is_component_events(godex::component_id p_component_id) {
	ERR_FAIL_COND_V_MSG(verify_component_id(p_component_id) == false, false, "The component " + itos(p_component_id) + " is invalid.");
	return components_info[p_component_id].is_event;
}

bool ECS::is_component_sharable(godex::component_id p_component_id) {
	ERR_FAIL_COND_V_MSG(verify_component_id(p_component_id) == false, false, "The component " + itos(p_component_id) + " is invalid.");
	return components_info[p_component_id].is_shareable;
}

bool ECS::storage_notify_release_write(godex::component_id p_component_id) {
	ERR_FAIL_COND_V_MSG(verify_component_id(p_component_id) == false, false, "The component " + itos(p_component_id) + " is invalid.");
	return components_info[p_component_id].notify_release_write;
}

const LocalVector<PropertyInfo> *ECS::get_component_properties(uint32_t p_component_id) {
	ERR_FAIL_COND_V_MSG(verify_component_id(p_component_id) == false, nullptr, "The `component_id` is invalid: " + itos(p_component_id));
	if (components_info[p_component_id].dynamic_component_info != nullptr) {
		return components_info[p_component_id].dynamic_component_info->get_properties();
	} else {
		return components_info[p_component_id].accessor_funcs.get_properties();
	}
}

Variant ECS::get_component_property_default(uint32_t p_component_id, StringName p_property_name) {
	ERR_FAIL_COND_V_MSG(verify_component_id(p_component_id) == false, Variant(), "The component " + itos(p_component_id) + " is invalid.");

	if (components_info[p_component_id].dynamic_component_info != nullptr) {
		// Script
		return components_info[p_component_id].dynamic_component_info->get_property_default(p_property_name).duplicate(true);
	} else {
		// Native
		return components_info[p_component_id].accessor_funcs.get_property_default(p_property_name);
	}
}

const LocalVector<godex::spawner_id> &ECS::get_spawners(godex::component_id p_component_id) {
	static const LocalVector<godex::spawner_id> invalid_return;
	ERR_FAIL_COND_V_MSG(ECS::verify_component_id(p_component_id) == false, invalid_return, "This component_id " + itos(p_component_id) + " is not valid.");
	return components_info[p_component_id].spawners;
}

bool ECS::unsafe_component_set_by_name(godex::component_id p_component_id, void *p_component, const StringName &p_name, const Variant &p_data) {
	if (components_info[p_component_id].dynamic_component_info) {
		// This is a dynamic component.
		return DynamicComponentInfo::static_set(p_component, components_info[p_component_id].dynamic_component_info, p_name, p_data);
	} else {
		return components_info[p_component_id].accessor_funcs.set_by_name(p_component, p_name, p_data);
	}
}

bool ECS::unsafe_component_get_by_name(godex::component_id p_component_id, const void *p_component, const StringName &p_name, Variant &r_data) {
	if (components_info[p_component_id].dynamic_component_info) {
		// This is a dynamic component.
		return DynamicComponentInfo::static_get(p_component, components_info[p_component_id].dynamic_component_info, p_name, r_data);
	} else {
		return components_info[p_component_id].accessor_funcs.get_by_name(p_component, p_name, r_data);
	}
}

Variant ECS::unsafe_component_get_by_name(godex::component_id p_component_id, const void *p_component, const StringName &p_name) {
	Variant r;
	unsafe_component_get_by_name(p_component_id, p_component, p_name, r);
	return r;
}

bool ECS::unsafe_component_set_by_index(godex::component_id p_component_id, void *p_component, uint32_t p_index, const Variant &p_data) {
	if (components_info[p_component_id].dynamic_component_info) {
		return DynamicComponentInfo::static_set(p_component, components_info[p_component_id].dynamic_component_info, p_index, p_data);
	} else {
		return components_info[p_component_id].accessor_funcs.set_by_index(p_component, p_index, p_data);
	}
}

bool ECS::unsafe_component_get_by_index(godex::component_id p_component_id, const void *p_component, uint32_t p_index, Variant &r_data) {
	if (components_info[p_component_id].dynamic_component_info) {
		return DynamicComponentInfo::static_get(p_component, components_info[p_component_id].dynamic_component_info, p_index, r_data);
	} else {
		return components_info[p_component_id].accessor_funcs.get_by_index(p_component, p_index, r_data);
	}
}

void ECS::unsafe_component_call(godex::component_id p_component_id, void *p_component, const StringName &p_method, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error) {
	if (components_info[p_component_id].dynamic_component_info != nullptr) {
		// This is a variant component, right now it's
		ERR_PRINT("GDScript component doesn't supports functions call (yet?).");
	} else {
		components_info[p_component_id].accessor_funcs.call(
				p_component,
				p_method,
				p_args,
				p_argcount,
				r_ret,
				r_error);
	}
}

bool ECS::verify_databag_id(godex::databag_id p_id) {
	return p_id < databags.size();
}

godex::Databag *ECS::create_databag(godex::databag_id p_id) {
#ifdef DEBUG_ENABLED
	// Crash cond because this function is not supposed to fail in any way.
	CRASH_COND_MSG(ECS::verify_databag_id(p_id) == false, "This databag id " + itos(p_id) + " is not valid.");
#endif
	return databags_info[p_id].create_databag();
}

uint32_t ECS::get_databag_count() {
	return databags.size();
}

uint32_t ECS::get_databag_id(const StringName &p_name) {
	const int64_t id = databags.find(p_name);
	return id >= 0 ? godex::databag_id(id) : UINT32_MAX;
}

StringName ECS::get_databag_name(godex::databag_id p_databag_id) {
	ERR_FAIL_COND_V_MSG(verify_databag_id(p_databag_id) == false, StringName(), "The `databag_id` is invalid: " + itos(p_databag_id));
	return databags[p_databag_id];
}

bool ECS::unsafe_databag_set_by_name(godex::databag_id p_databag_id, void *p_databag, const StringName &p_name, const Variant &p_data) {
	return databags_info[p_databag_id].accessor_funcs.set_by_name(p_databag, p_name, p_data);
}

bool ECS::unsafe_databag_get_by_name(godex::databag_id p_databag_id, const void *p_databag, const StringName &p_name, Variant &r_data) {
	return databags_info[p_databag_id].accessor_funcs.get_by_name(p_databag, p_name, r_data);
}

Variant ECS::unsafe_databag_get_by_name(godex::databag_id p_databag_id, const void *p_databag, const StringName &p_name) {
	Variant r;
	unsafe_databag_get_by_name(p_databag_id, p_databag, p_name, r);
	return r;
}

bool ECS::unsafe_databag_set_by_index(godex::databag_id p_databag_id, void *p_databag, uint32_t p_index, const Variant &p_data) {
	return databags_info[p_databag_id].accessor_funcs.set_by_index(p_databag, p_index, p_data);
}

bool ECS::unsafe_databag_get_by_index(godex::databag_id p_databag_id, const void *p_databag, uint32_t p_index, Variant &r_data) {
	return databags_info[p_databag_id].accessor_funcs.get_by_index(p_databag, p_index, r_data);
}

void ECS::unsafe_databag_call(
		godex::databag_id p_databag_id,
		void *p_databag,
		const StringName &p_method,
		const Variant **p_args,
		int p_argcount,
		Variant *r_ret,
		Callable::CallError &r_error) {
	databags_info[p_databag_id].accessor_funcs.call(
			p_databag,
			p_method,
			p_args,
			p_argcount,
			r_ret,
			r_error);
}

SystemBundleInfo &ECS::register_system_bundle(const StringName &p_name) {
	{
		const uint32_t id = get_system_bundle_id(p_name);
		if (id != godex::SYSTEM_BUNDLE_NONE) {
			// The system bundle is already registered.
			system_bundles_info[id].reset();
			return system_bundles_info[id];
		}
	}

	const godex::system_bundle_id id = system_bundles.size();
	system_bundles.push_back(p_name);
	system_bundles_info.push_back(SystemBundleInfo());

	print_line("SystemBundle: " + p_name + " registered with ID: " + itos(id));
	return system_bundles_info[id];
}

godex::system_bundle_id ECS::get_system_bundle_id(const StringName &p_name) {
	system_bundles.find(p_name);
	const int64_t index = system_bundles.find(p_name);
	return index >= 0 ? godex::system_bundle_id(index) : godex::SYSTEM_BUNDLE_NONE;
}

SystemBundleInfo &ECS::get_system_bundle(godex::system_bundle_id p_id) {
	CRASH_COND_MSG(system_bundles.size() <= p_id, "The sysetm bundle " + itos(p_id) + " doesn't exists.");
	return system_bundles_info[p_id];
}

// Undefine the macro defined into `ecs.h` so we can define the method properly.
#undef register_system
SystemInfo &ECS::register_system(func_get_system_exe_info p_func_get_exe_info, StringName p_name) {
	{
		const uint32_t id = get_system_id(p_name);
		CRASH_COND_MSG(id != UINT32_MAX, "The system is already registered.");
	}

	const godex::system_id id = systems.size();
	systems.push_back(p_name);
	systems_info.push_back(SystemInfo());
	systems_info[id].id = id;
	systems_info[id].exec_info = p_func_get_exe_info;

	ClassDB::bind_integer_constant(get_class_static(), StringName(), p_name, id);
	print_line("System: " + p_name + " registered with ID: " + itos(id));
	return systems_info[id];
}

SystemInfo &ECS::register_dynamic_system(StringName p_name) {
	{
		const uint32_t id = get_system_id(p_name);
		if (id != godex::SYSTEM_NONE) {
			// The system is already registered, if it's a dynamic system reset it.
			CRASH_COND_MSG(systems_info[id].dynamic_system_id == UINT32_MAX, "You can't register a dynamic system with the same name of a static one. Name used: " + p_name);
			godex::DynamicSystemInfo *si = godex::get_dynamic_system_info(systems_info[id].dynamic_system_id);
			si->reset();
			si->set_system_id(id);
			return systems_info[id];
		}
	}

	const godex::system_id id = systems.size();

	// Used to assign a static function to this dynamic system, check the
	// DynamicSystem doc to know more (../systems/dynamic_system.h).
	const uint32_t dynamic_system_id = godex::register_dynamic_system();

	systems.push_back(p_name);
	systems_info.push_back(SystemInfo());

	systems_info[id].id = id;
	systems_info[id].dynamic_system_id = dynamic_system_id;
	systems_info[id].exec_info = godex::get_func_dynamic_system_exec_info(dynamic_system_id);

	godex::get_dynamic_system_info(dynamic_system_id)->set_system_id(id);

	ClassDB::bind_integer_constant(get_class_static(), StringName(), String(p_name).replace(".", "_"), id);
	print_line("Dynamic system: " + p_name + " registered with ID: " + itos(id));

	return systems_info[id];
}

godex::system_id ECS::get_system_id(const StringName &p_name) {
	const int64_t index = systems.find(p_name);
	return index >= 0 ? godex::system_id(index) : UINT32_MAX;
}

uint32_t ECS::get_systems_count() {
	return systems.size();
}

func_get_system_exe_info ECS::get_func_system_exe_info(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, nullptr, "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	return systems_info[p_id].exec_info;
}

void ECS::get_system_exe_info(godex::system_id p_id, SystemExeInfo &r_info) {
	ERR_FAIL_COND_MSG(verify_system_id(p_id) == false, "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	ERR_FAIL_COND_MSG(systems_info[p_id].exec_info == nullptr, "The System " + systems[p_id] + " is not a standard `System`.");
	return systems_info[p_id].exec_info(r_info);
}

StringName ECS::get_system_name(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, StringName(), "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	return systems[p_id];
}

String ECS::get_system_desc(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, String(), "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	return systems_info[p_id].description;
}

void ECS::set_dynamic_system_target(godex::system_id p_id, ScriptInstance *p_target) {
	ERR_FAIL_COND_MSG(verify_system_id(p_id) == false, "This system " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	ERR_FAIL_COND_MSG(systems_info[p_id].dynamic_system_id == UINT32_MAX, "The system " + itos(p_id) + " is not a dynamic system.");
	godex::DynamicSystemInfo *info = godex::get_dynamic_system_info(systems_info[p_id].dynamic_system_id);
	info->set_target(p_target);
}

godex::DynamicSystemInfo *ECS::get_dynamic_system_info(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, nullptr, "This system " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	ERR_FAIL_COND_V_MSG(systems_info[p_id].dynamic_system_id == UINT32_MAX, nullptr, "The system " + itos(p_id) + " is not a dynamic system.");
	return godex::get_dynamic_system_info(systems_info[p_id].dynamic_system_id);
}

bool ECS::is_system_dispatcher(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, false, "This system " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	if (systems_info[p_id].dynamic_system_id == UINT32_MAX) {
		// Only dynamic systems are pipeline_dispatchers.
		return false;
	}
	godex::DynamicSystemInfo *info = godex::get_dynamic_system_info(systems_info[p_id].dynamic_system_id);
	return info->is_system_dispatcher();
}

void ECS::set_system_pipeline(godex::system_id p_id, Pipeline *p_pipeline) {
	ERR_FAIL_COND_MSG(verify_system_id(p_id) == false, "This system " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	ERR_FAIL_COND_MSG(systems_info[p_id].dynamic_system_id == UINT32_MAX, "The system " + itos(p_id) + " is not a dynamic system.");
	godex::DynamicSystemInfo *info = godex::get_dynamic_system_info(systems_info[p_id].dynamic_system_id);
	ERR_FAIL_COND_MSG(info->is_system_dispatcher() == false, "The system " + itos(p_id) + " is not a sub pipeline dispatcher.");
	info->set_pipeline(p_pipeline);
}

// Undefine the macro defined into `ecs.h` so we can define the method properly.
#undef register_temporary_system
SystemInfo &ECS::register_temporary_system(func_temporary_system_execute p_func_temporary_systems_exe, StringName p_name) {
	{
		const uint32_t id = get_system_id(p_name);
		CRASH_COND_MSG(id != UINT32_MAX, "The system is already registered.");
	}

	const godex::system_id id = systems.size();
	systems.push_back(p_name);
	systems_info.push_back(SystemInfo());
	systems_info[id].id = id;
	systems_info[id].temporary_exec = p_func_temporary_systems_exe;

	print_line("TemporarySystem: " + p_name + " registered with ID: " + itos(id));

	return systems_info[id];
}

bool ECS::is_temporary_system(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, false, "The TemporarySystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	return systems_info[p_id].temporary_exec != nullptr;
}

func_temporary_system_execute ECS::get_func_temporary_system_exe(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, nullptr, "The TemporarySystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	ERR_FAIL_COND_V_MSG(systems_info[p_id].temporary_exec == nullptr, nullptr, "The System : " + systems[p_id] + " is not a TemporarySystem.");
	return systems_info[p_id].temporary_exec;
}

bool ECS::verify_system_id(godex::system_id p_id) {
	return systems.size() > p_id;
}

ECS *ECS::get_singleton() {
	return singleton;
}

void ECS::__set_singleton(ECS *p_singleton) {
	if (p_singleton == nullptr) {
		ERR_FAIL_COND(singleton == nullptr);
		singleton = nullptr;
	} else {
		ERR_FAIL_COND_MSG(singleton != nullptr, "There is already a singleton, make sure to remove that first.");
		singleton = p_singleton;
	}
}

void ECS::set_active_world(World *p_world, WorldECS *p_active_world_ecs) {
	if (active_world != nullptr) {
		if (p_world == nullptr) {
			if (active_world_node->is_inside_tree() == false) {
				ERR_PRINT("The current active world is already no more in tree, this is a bug.");
			} else {
				active_world_node->get_tree()->get_root()->propagate_notification(NOTIFICATION_ECS_WORLD_PRE_UNLOAD);
			}
		} else {
			ERR_FAIL_COND("Before adding a new world it's necessary remove the current one by calling `set_active_world(nullptr);`.");
		}
	}

	WorldECS *prev_node = active_world_node;

	active_world = p_world;
	active_world_node = p_active_world_ecs;
	ready = false;
	active_world_pipeline = nullptr;

	if (active_world != nullptr) {
		// The world is just loaded.
		if (active_world_node->is_inside_tree() == false) {
			ERR_PRINT("The new active world is not in tree, this is a bug.");
		} else {
			active_world_node->get_tree()->get_root()->propagate_notification(NOTIFICATION_ECS_WORLD_LOADED);
		}
	} else {
		// The world is just unloaded.
		if (prev_node) {
			if (prev_node->is_inside_tree() == false) {
				ERR_PRINT("The previous world is already not in tree, this is a bug.");
			} else {
				prev_node->get_tree()->get_root()->propagate_notification(NOTIFICATION_ECS_WORLD_UNLOADED);
			}
		}
	}
}

World *ECS::get_active_world() {
	return active_world;
}

const World *ECS::get_active_world() const {
	return active_world;
}

Node *ECS::get_active_world_node() {
	return active_world_node;
}

bool ECS::has_active_world() const {
	return active_world != nullptr;
}

bool ECS::is_world_ready() const {
	return has_active_world() && ready;
}

void ECS::set_active_world_pipeline(Pipeline *p_pipeline) {
#ifdef DEBUG_ENABLED
	if (p_pipeline) {
		// Using crash cond because the user doesn't never use this API directly.
		CRASH_COND_MSG(p_pipeline->is_ready() == false, "The submitted pipeline is not fully build.");
	}
#endif
	active_world_pipeline = p_pipeline;

	if (active_world_pipeline && active_world) {
		active_world_pipeline->prepare(active_world);
	}
}

Pipeline *ECS::get_active_world_pipeline() const {
	return active_world_pipeline;
}

bool ECS::has_active_world_pipeline() const {
	return active_world_pipeline != nullptr;
}

void ECS::dispatch_active_world() {
	if (likely(active_world && active_world_pipeline)) {
		if (unlikely(ready == false)) {
			// Ready.
			active_world_node->get_tree()->get_root()->propagate_notification(NOTIFICATION_ECS_WORLD_READY);
			ready = true;
		}

		active_world_node->pre_process();

		dispatching = true;
		active_world_pipeline->dispatch(active_world);
		active_world->flush();
		dispatching = false;

		active_world_node->post_process();
	}
}

void ECS::ecs_init() {
}

uint32_t ECS::get_spawners_count() {
	return spawners.size();
}

bool ECS::verify_spawner_id(godex::spawner_id p_spawner_id) {
	return p_spawner_id < spawners.size();
}

godex::spawner_id ECS::get_spawner_id(const StringName &p_name) {
	const int64_t index = spawners.find(p_name);
	return index >= 0 ? index : godex::SPAWNER_NONE;
}

StringName ECS::get_spawner_name(godex::spawner_id p_spawner) {
	ERR_FAIL_COND_V_MSG(ECS::verify_spawner_id(p_spawner) == false, StringName(), "This spawner_id " + itos(p_spawner) + " is not valid.");
	return spawners[p_spawner];
}

const LocalVector<godex::component_id> &ECS::get_spawnable_components(godex::spawner_id p_spawner) {
	static const LocalVector<godex::component_id> invalid_return;
	ERR_FAIL_COND_V_MSG(ECS::verify_spawner_id(p_spawner) == false, invalid_return, "This spawner_id " + itos(p_spawner) + " is not valid.");
	return spawners_info[p_spawner].components;
}

uint32_t ECS::register_or_update_script_component(
		const StringName &p_name,
		const LocalVector<ScriptProperty> &p_properties,
		StorageType p_storage_type,
		Vector<StringName> p_spawners) {
	godex::component_id id = get_component_id(p_name);
	DynamicComponentInfo *info;

	if (id == godex::COMPONENT_NONE) {
		// This is a new component.
		id = components_info.size();
		info = memnew(DynamicComponentInfo);
		info->component_id = id;

		components.push_back(p_name);
		components_info.push_back(ComponentInfo());
		components_info[id].dynamic_component_info = info;
	} else {
		// This is an old component, verify is a script component.
		ERR_FAIL_COND_V_MSG(components_info[id].dynamic_component_info == nullptr, godex::COMPONENT_NONE, "This component " + p_name + " is not a script component and can't be updated. Your component must be an unique name.");
		info = components_info[id].dynamic_component_info;
	}

	info->property_map.resize(p_properties.size());
	info->properties.resize(p_properties.size());
	info->defaults.resize(p_properties.size());

	// Validate and initialize the parameters.
	for (uint32_t i = 0; i < p_properties.size(); i += 1) {
		// Is  type supported?
		switch (p_properties[i].property.type) {
			case Variant::NIL:
			case Variant::RID:
			case Variant::OBJECT:
			case Variant::SIGNAL:
			case Variant::CALLABLE:
				// TODO what about dictionary and arrays?
				ERR_PRINT("The script component " + p_name + " is using a pointer variable. This is unsafe, so not supported. Please use a databag.");
				return UINT32_MAX;
			default:
				// Valid!
				break;
		}

		// Is default type correct?
		if (p_properties[i].property.type != p_properties[i].default_value.get_type()) {
			ERR_PRINT("The script variable " + p_name + "::" + p_properties[i].property.name + " is being initialized with a different default variable type.");
			return UINT32_MAX;
		}

		info->property_map[i] = p_properties[i].property.name;
		info->properties[i] = p_properties[i].property;
		info->defaults[i] = p_properties[i].default_value;
	}

	info->storage_type = p_storage_type;

	// Extract the spawners.
	components_info[id].spawners.clear();
	for (int i = 0; i < p_spawners.size(); i += 1) {
		const godex::spawner_id spawner = get_spawner_id(p_spawners[i]);
		ERR_CONTINUE_MSG(ECS::verify_spawner_id(spawner) == false, "The script component " + p_name + " has an invalid spawner with name: " + p_spawners[i]);

		components_info[id].spawners.push_back(spawner);
		spawners_info[spawner].components.push_back(info->component_id);
	}

	// Add a new scripting constant, for fast and easy `component` access.
	ClassDB::bind_integer_constant(get_class_static(), StringName(), String(p_name).replace(".", "_"), id);
	print_line("ComponentScript: " + p_name + " registered with ID: " + itos(id));

	return id;
}

uint32_t ECS::register_or_update_script_component_event(
		const StringName &p_name,
		const LocalVector<ScriptProperty> &p_properties,
		StorageType p_storage_type,
		Vector<StringName> p_spawners) {
	const uint32_t cid = register_or_update_script_component(p_name, p_properties, p_storage_type, p_spawners);
	ERR_FAIL_COND_V(cid == UINT32_MAX, UINT32_MAX);

	components_info[cid].is_event = true;
	return cid;
}

uint32_t ECS::get_components_count() {
	return components.size();
}

bool ECS::verify_component_id(uint32_t p_component_id) {
	return components.size() > p_component_id;
}

StorageBase *ECS::create_storage(uint32_t p_component_id) {
#ifdef DEBUG_ENABLED
	// Crash cond because this function is not supposed to fail in any way.
	CRASH_COND_MSG(ECS::verify_component_id(p_component_id) == false, "This component id " + itos(p_component_id) + " is not valid. Are you passing a Component ID?");
#endif
	if (components_info[p_component_id].dynamic_component_info) {
		// This is a script component
		return components_info[p_component_id].dynamic_component_info->create_storage();
	} else {
		// This is a native component.
		return components_info[p_component_id].create_storage();
	}
}

void *ECS::new_component(godex::component_id p_component_id) {
	ERR_FAIL_COND_V_MSG(ECS::verify_component_id(p_component_id) == false, nullptr, "This component id " + itos(p_component_id) + " is not valid. Are you passing a Component ID?");
	return components_info[p_component_id].new_component();
}

void ECS::free_component(godex::component_id p_component_id, void *p_component) {
	ERR_FAIL_COND_MSG(ECS::verify_component_id(p_component_id) == false, "This component id " + itos(p_component_id) + " is not valid. Are you passing a Component ID?");
	components_info[p_component_id].free_component(p_component);
}

void ECS::get_storage_config(godex::component_id p_component_id, Dictionary &r_config) {
	ERR_FAIL_COND_MSG(ECS::verify_component_id(p_component_id) == false, "This component doesn't exists: " + itos(p_component_id) + ". Are you passing a Component ID?");
	if (components_info[p_component_id].get_storage_config != nullptr) {
		components_info[p_component_id].get_storage_config(r_config);
	}
}
