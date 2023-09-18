
#include "ecs.h"
#include <core/config/project_settings.h>
#include <core/io/dir_access.h>
#include <modules/gdscript/gdscript_parser.h>

#include "components/dynamic_component.h"
#include "core/object/message_queue.h"
#include "modules/godot/databags/scene_tree_databag.h"
#include "modules/godot/nodes/ecs_utilities.h"
#include "modules/godot/nodes/ecs_world.h"
#include "pipeline/pipeline.h"
#include "pipeline/pipeline_commands.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "systems/dynamic_system.h"
#include "world/world.h"

ECS *ECS::singleton = nullptr;
int ECS::dispatcher_count = 1; // Starts from 1 to reserve 0 to the main dispatcher.
LocalVector<func_notify_static_destructor> ECS::notify_static_destructor;
LocalVector<StringName> ECS::spawners;
LocalVector<SpawnerInfo> ECS::spawners_info;
LocalVector<StringName> ECS::components;
LocalVector<ComponentInfo> ECS::components_info;
LocalVector<StringName> ECS::databags;
LocalVector<DatabagInfo> ECS::databags_info;
LocalVector<StringName> ECS::events;
LocalVector<EventInfo> ECS::events_info;
LocalVector<StringName> ECS::systems;
LocalVector<SystemInfo> ECS::systems_info;
LocalVector<StringName> ECS::system_bundles;
LocalVector<SystemBundleInfo> ECS::system_bundles_info;

godex::system_id SystemInfo::get_id() const {
	return id;
}

SystemInfo &SystemInfo::execute_in(Phase p_phase, const StringName &p_dispatcher_name) {
	phase = p_phase;
	dispatcher = p_dispatcher_name;
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

SystemInfo &SystemInfo::with_flags(int p_flags) {
	flags = p_flags;
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
	BIND_ENUM_CONSTANT(PHASE_PRE_RENDER);
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

	events.reset();
	events_info.reset();

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

void get_script_files(const String &p_path, Vector<String> &scripts) {
	for (const auto &directory : DirAccess::get_directories_at(p_path)) {
		get_script_files(p_path.path_join(directory), scripts);
	}
	for (const auto &file : DirAccess::get_files_at(p_path)) {
		auto file_path = p_path.path_join(file);
		if (ResourceLoader::get_resource_type(file_path) == "GDScript") {
			scripts.push_back(file_path);
		}
	}
}

void ECS::preload_scripts() {
	if (Engine::get_singleton()->is_project_manager_hint()) {
		return;
	}

	Vector<String> scripts;
	print_line("Preloading component and system scripts");
	get_script_files("res://", scripts);

	for (const auto &script : scripts) {
		auto code = FileAccess::get_file_as_string(script);
		GDScriptParser parser;
		if (parser.parse(code, script, false) == OK) {
			auto tree = parser.get_tree();
			if (tree->extends.has("System")) {
				register_dynamic_system(script.get_file());
			}

			if (tree->extends.has("Component")) {
				register_or_get_id_for_component_name(script.get_file());
			}
		}
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

bool ECS::is_component_sharable(godex::component_id p_component_id) {
	ERR_FAIL_COND_V_MSG(verify_component_id(p_component_id) == false, false, "The component " + itos(p_component_id) + " is invalid.");
	return components_info[p_component_id].is_shareable;
}

bool ECS::storage_notify_release_write(godex::component_id p_component_id) {
	ERR_FAIL_COND_V_MSG(verify_component_id(p_component_id) == false, false, "The component " + itos(p_component_id) + " is invalid.");
	return components_info[p_component_id].notify_release_write;
}

const LocalVector<PropertyInfo> *ECS::component_get_static_properties(uint32_t p_component_id) {
	ERR_FAIL_COND_V_MSG(verify_component_id(p_component_id) == false, nullptr, "The `component_id` is invalid: " + itos(p_component_id));
	if (components_info[p_component_id].dynamic_component_info != nullptr) {
		return components_info[p_component_id].dynamic_component_info->get_static_properties();
	} else {
		return components_info[p_component_id].accessor_funcs.get_static_properties();
	}
}

void ECS::unsafe_component_get_property_list(godex::component_id p_component_id, void *p_component, List<PropertyInfo> *r_list) {
	if (components_info[p_component_id].dynamic_component_info != nullptr) {
		return DynamicComponentInfo::get_property_list(p_component, components_info[p_component_id].dynamic_component_info, r_list);
	} else {
		return components_info[p_component_id].accessor_funcs.get_property_list(p_component, r_list);
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
		ERR_PRINT("GDScript component doesn't support function calls (yet?).");
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

void ECS::unsafe_databag_get_property_list(godex::databag_id p_databag_id, void *p_databag, List<PropertyInfo> *r_list) {
	databags_info[p_databag_id].accessor_funcs.get_property_list(p_databag, r_list);
}

bool ECS::verify_event_id(godex::event_id p_id) {
	return p_id < events.size();
}

godex::event_id ECS::get_event_id(const StringName &p_name) {
	const int64_t index = events.find(p_name);
	return index >= 0 ? index : UINT32_MAX;
}

StringName ECS::get_event_name(godex::event_id p_event_id) {
	ERR_FAIL_COND_V_MSG(ECS::verify_event_id(p_event_id) == false, StringName(), "This event id " + itos(p_event_id) + " is not valid. Are you passing an Event ID?");
	return events[p_event_id];
}

EventStorageBase *ECS::create_events_storage(godex::event_id p_event_id) {
#ifdef DEBUG_ENABLED
	// Crash cond because this function is not supposed to fail in any way.
	CRASH_COND_MSG(ECS::verify_event_id(p_event_id) == false, "This event id " + itos(p_event_id) + " is not valid. Are you passing an Event ID?");
#endif
	// if (event_indo[p_event_id].dynamic_event_info) {
	//	// This is a script event
	//	return events_info[p_event_id].dynamic_event_info->create_storage();
	// } else {
	//  This is a native event.
	return events_info[p_event_id].create_storage();
	//}
}

void ECS::destroy_events_storage(godex::event_id p_event_id, EventStorageBase *p_storage) {
#ifdef DEBUG_ENABLED
	// Crash cond because this function is not supposed to fail in any way.
	CRASH_COND_MSG(ECS::verify_event_id(p_event_id) == false, "This event id " + itos(p_event_id) + " is not valid. Are you passing an Event ID?");
#endif
	if (p_storage == nullptr) {
		// Nothing to do.
		return;
	}

	// if (event_indo[p_event_id].dynamic_event_info) {
	//	// This is a script event
	//	return events_info[p_event_id].dynamic_event_info->destroy_storage(p_storage);
	// } else {
	//  This is a native event.
	return events_info[p_event_id].destroy_storage(p_storage);
	//}
}

const RBSet<String> &ECS::get_event_emitters(godex::event_id p_event_id) {
#ifdef DEBUG_ENABLED
	CRASH_COND_MSG(ECS::verify_event_id(p_event_id) == false, "This event id " + itos(p_event_id) + " is not valid. Are you passing an Event ID?");
#endif

	if (events_info[p_event_id].event_emitters_need_reload) {
		events_info[p_event_id].event_emitters_need_reload = false;
		events_info[p_event_id].event_emitters.clear();

		// Fetch the available emitters.
		// The emitters are defined by the systems that receive the event: for this
		// reason here we are checking the `events_receivers`.
		SystemExeInfo info;
		for (uint32_t i = 0; i < systems_info.size(); i += 1) {
			info.clear();
			ECS::get_system_exe_info(i, info);

			const RBSet<String> *it = info.events_receivers.lookup_ptr(p_event_id);
			if (it) {
				for (const RBSet<String>::Element *emitter_name = it->front(); emitter_name; emitter_name = emitter_name->next()) {
					events_info[p_event_id].event_emitters.insert(emitter_name->get());
				}
			}
		}
	}

	return events_info[p_event_id].event_emitters;
}

bool ECS::unsafe_event_set_by_name(godex::event_id p_event_id, void *p_event, const StringName &p_name, const Variant &p_data) {
	return events_info[p_event_id].accessor_funcs.set_by_name(p_event, p_name, p_data);
}

bool ECS::unsafe_event_get_by_name(godex::event_id p_event_id, const void *p_event, const StringName &p_name, Variant &r_data) {
	return events_info[p_event_id].accessor_funcs.get_by_name(p_event, p_name, r_data);
}

Variant ECS::unsafe_event_get_by_name(godex::event_id p_event_id, const void *p_event, const StringName &p_name) {
	Variant ret;
	ECS::unsafe_event_get_by_name(p_event_id, p_event, p_name, ret);
	return ret;
}

bool ECS::unsafe_event_set_by_index(godex::event_id p_event_id, void *p_event, uint32_t p_index, const Variant &p_data) {
	return events_info[p_event_id].accessor_funcs.set_by_index(p_event, p_index, p_data);
}

bool ECS::unsafe_event_get_by_index(godex::event_id p_event_id, const void *p_event, uint32_t p_index, Variant &r_data) {
	return events_info[p_event_id].accessor_funcs.get_by_index(p_event, p_index, r_data);
}

void ECS::unsafe_event_call(godex::event_id p_event_id, void *p_event, const StringName &p_method, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error) {
	events_info[p_event_id].accessor_funcs.call(p_event, p_method, p_args, p_argcount, r_ret, r_error);
}

void ECS::unsafe_event_get_property_list(godex::event_id p_event_id, void *p_event, List<PropertyInfo> *r_list) {
	events_info[p_event_id].accessor_funcs.get_property_list(p_event, r_list);
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

uint32_t ECS::get_system_bundle_count() {
	return system_bundles.size();
}

godex::system_bundle_id ECS::get_system_bundle_id(const StringName &p_name) {
	const int64_t index = system_bundles.find(p_name);
	return index >= 0 ? godex::system_bundle_id(index) : godex::SYSTEM_BUNDLE_NONE;
}

StringName ECS::get_system_bundle_name(godex::system_bundle_id p_id) {
	ERR_FAIL_COND_V_MSG(system_bundles.size() <= p_id, StringName(), "The sysetm bundle " + itos(p_id) + " doesn't exists.");
	return system_bundles[p_id];
}

const String &ECS::get_system_bundle_desc(godex::system_bundle_id p_id) {
	static const String err_ret;
	ERR_FAIL_COND_V_MSG(system_bundles.size() <= p_id, err_ret, "The sysetm bundle " + itos(p_id) + " doesn't exists.");
	return system_bundles_info[p_id].description;
}

uint32_t ECS::get_system_bundle_systems_count(godex::system_bundle_id p_id) {
	ERR_FAIL_COND_V_MSG(system_bundles.size() <= p_id, 0, "The sysetm bundle " + itos(p_id) + " doesn't exists.");
	return system_bundles_info[p_id].systems.size();
}

SystemBundleInfo &ECS::get_system_bundle(godex::system_bundle_id p_id) {
	CRASH_COND_MSG(system_bundles.size() <= p_id, "The sysetm bundle " + itos(p_id) + " doesn't exists.");
	return system_bundles_info[p_id];
}

// Undefine the macro defined into `ecs.h` so we can define the method properly.
#undef register_system
SystemInfo &ECS::register_system(
		func_get_system_exe_info p_func_get_exe_info,
		func_system_data_get_size p_system_data_get_size,
		func_system_data_new_placement p_system_data_new_placement,
		func_system_data_delete_placement p_system_data_delete_placement,
		func_system_data_set_active p_system_data_set_active,
		StringName p_name) {
	{
		const uint32_t id = get_system_id(p_name);
		CRASH_COND_MSG(id != UINT32_MAX, "The system is already registered.");
	}

	const godex::system_id id = systems.size();

	systems.push_back(p_name);
	systems_info.push_back(SystemInfo());

	systems_info[id].id = id;
	systems_info[id].exec_info = p_func_get_exe_info;

	systems_info[id].system_data_get_size = p_system_data_get_size;
	systems_info[id].system_data_new_placement = p_system_data_new_placement;
	systems_info[id].system_data_delete_placement = p_system_data_delete_placement;
	systems_info[id].system_data_set_active = p_system_data_set_active;

	ClassDB::bind_integer_constant(get_class_static(), StringName(), p_name, id);
	print_line("System: " + p_name + " registered with ID: " + itos(id));
	return systems_info[id];
}

#undef register_system_dispatcher
SystemInfo &ECS::register_system_dispatcher(
		func_get_system_exe_info p_func_get_exe_info,
		func_system_data_get_size p_system_data_get_size,
		func_system_data_new_placement p_system_data_new_placement,
		func_system_data_delete_placement p_system_data_delete_placement,
		func_system_data_set_active p_system_data_set_active,
		StringName p_name) {
	{
		const uint32_t id = get_system_id(p_name);
		CRASH_COND_MSG(id != UINT32_MAX, "The system is already registered.");
	}

	const godex::system_id id = systems.size();

	systems.push_back(p_name);
	systems_info.push_back(SystemInfo());

	systems_info[id].id = id;
	systems_info[id].exec_info = p_func_get_exe_info;
	systems_info[id].type = SystemInfo::TYPE_DISPATCHER;
	systems_info[id].dispatcher_index = dispatcher_count;

	systems_info[id].system_data_get_size = p_system_data_get_size;
	systems_info[id].system_data_new_placement = p_system_data_new_placement;
	systems_info[id].system_data_delete_placement = p_system_data_delete_placement;
	systems_info[id].system_data_set_active = p_system_data_set_active;

	dispatcher_count += 1;

	ClassDB::bind_integer_constant(get_class_static(), StringName(), p_name, id);
	print_line("System Dispatcher: " + p_name + " registered with ID: " + itos(id));
	return systems_info[id];
}

// This function is used by the dispatcher systems, to process specific pipeline
// dispatchers.
void ECS::__process_pipeline_dispatcher(
		uint32_t p_count,
		Token p_token,
		Pipeline *p_pipeline,
		int p_dispatcher_index) {
	for (uint32_t i = 0; i < p_count; i += 1) {
		p_pipeline->dispatch_sub_dispatcher(
				p_token,
				p_dispatcher_index);
	}
}

// Undefine the macro defined into `ecs.h` so we can define the method properly.
#undef register_temporary_system
SystemInfo &ECS::register_temporary_system(
		func_temporary_system_execute p_func_temporary_systems_exe,
		func_system_data_get_size p_system_data_get_size,
		func_system_data_new_placement p_system_data_new_placement,
		func_system_data_delete_placement p_system_data_delete_placement,
		func_system_data_set_active p_system_data_set_active,
		StringName p_name) {
	{
		const uint32_t id = get_system_id(p_name);
		CRASH_COND_MSG(id != UINT32_MAX, "The system is already registered.");
	}

	const godex::system_id id = systems.size();
	systems.push_back(p_name);
	systems_info.push_back(SystemInfo());
	systems_info[id].id = id;
	systems_info[id].temporary_exec = p_func_temporary_systems_exe;
	systems_info[id].type = SystemInfo::TYPE_TEMPORARY;

	systems_info[id].system_data_get_size = p_system_data_get_size;
	systems_info[id].system_data_new_placement = p_system_data_new_placement;
	systems_info[id].system_data_delete_placement = p_system_data_delete_placement;
	systems_info[id].system_data_set_active = p_system_data_set_active;

	print_line("TemporarySystem: " + p_name + " registered with ID: " + itos(id));

	return systems_info[id];
}

SystemInfo &ECS::register_dynamic_system(StringName p_name) {
	{
		const uint32_t id = get_system_id(p_name);
		if (id != godex::SYSTEM_NONE) {
			// The system is already registered, if it's a dynamic system reset it.
			clear_emitters_for_system(id);
			systems_info[id].phase = PHASE_PROCESS;
			systems_info[id].dependencies.reset();
			systems_info[id].type = SystemInfo::TYPE_DYNAMIC;
			// Make sure to clear any previously fetched emitter.
			return systems_info[id];
		}
	}

	const godex::system_id id = systems.size();

	systems.push_back(p_name);
	systems_info.push_back(SystemInfo());

	systems_info[id].id = id;
	systems_info[id].type = SystemInfo::TYPE_DYNAMIC;
	systems_info[id].exec_info = System::get_system_exec_info;

	systems_info[id].system_data_get_size = System::dynamic_system_data_get_size;
	systems_info[id].system_data_new_placement = System::dynamic_system_data_new_placement;
	systems_info[id].system_data_delete_placement = System::dynamic_system_data_delete_placement;
	systems_info[id].system_data_set_active = System::dynamic_system_data_set_active;

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

bool has_single_thread_only_databags(const SystemExeInfo &p_info) {
	return
			// `PipelneCommands` is unsafe to execute only if mutable.
			p_info.mutable_databags.has(PipelineCommands::get_databag_id()) ||
			// `World` is always unsafe to execute in MT.
			p_info.immutable_databags.has(World::get_databag_id()) ||
			p_info.mutable_databags.has(World::get_databag_id()) ||
			// `SceneTreeDatabag` is always unsafe to execute in MT.
			p_info.immutable_databags.has(SceneTreeDatabag::get_databag_id()) ||
			p_info.mutable_databags.has(SceneTreeDatabag::get_databag_id());
}

/// Returns true if these two `Set`s have at least 1 ID in common.
bool collides(const RBSet<uint32_t> &p_set_1, const RBSet<uint32_t> &p_set_2) {
	for (RBSet<uint32_t>::Element *e = p_set_1.front(); e; e = e->next()) {
		if (p_set_2.has(e->get())) {
			return true;
		}
	}
	return false;
}

bool collides(const RBSet<uint32_t> &p_set_1, const OAHashMap<uint32_t, RBSet<String>> &p_map_2) {
	for (RBSet<uint32_t>::Element *e = p_set_1.front(); e; e = e->next()) {
		if (p_map_2.has(e->get())) {
			return true;
		}
	}
	return false;
}

bool ECS::can_systems_run_in_parallel(godex::system_id p_system_a, godex::system_id p_system_b) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_system_a) == false, false, "The SystemID: " + itos(p_system_a) + " doesn't exists. Are you passing a System ID?");
	ERR_FAIL_COND_V_MSG(verify_system_id(p_system_b) == false, false, "The SystemID: " + itos(p_system_b) + " doesn't exists. Are you passing a System ID?");

	SystemExeInfo info_a;
	SystemExeInfo info_b;

	get_system_exe_info(p_system_a, info_a);
	get_system_exe_info(p_system_b, info_b);

	// Verify if one of those has a special component or databag that must always
	// run in single thread even when taken immutable.
	if (has_single_thread_only_databags(info_a)) {
		return false;
	}
	if (has_single_thread_only_databags(info_b)) {
		return false;
	}

	// Check the remaining databags.
	if (collides(info_a.immutable_databags, info_b.mutable_databags)) {
		// System A is reading a databag mutating in System B.
		return false;
	}
	if (collides(info_a.mutable_databags, info_b.mutable_databags)) {
		// System A is mutating a databag mutating in System B.
		return false;
	}
	if (collides(info_b.immutable_databags, info_a.mutable_databags)) {
		// System B is reading a databag mutating in System A.
		return false;
	}

	// Check the component Storages.
	if (collides(info_a.immutable_components, info_b.mutable_components)) {
		// System A is reading a component storage mutating in System B.
		return false;
	}
	if (collides(info_a.immutable_components, info_b.mutable_components_storage)) {
		// System A is reading a component storage mutating in System B.
		return false;
	}
	if (collides(info_a.mutable_components, info_b.mutable_components)) {
		// System A is mutating a component storage mutating in System B.
		return false;
	}
	if (collides(info_a.mutable_components, info_b.mutable_components_storage)) {
		// System A is mutating a component storage mutating in System B.
		return false;
	}
	if (collides(info_b.immutable_components, info_a.mutable_components)) {
		// System B is reading a component storage mutating in System A.
		return false;
	}
	if (collides(info_b.immutable_components, info_a.mutable_components_storage)) {
		// System B is reading a component storage mutating in System A.
		return false;
	}

	// Check the events
	if (collides(info_a.events_emitters, info_b.events_emitters)) {
		// System A is emitting the same event the System B is emitting.
		return false;
	}
	if (collides(info_a.events_emitters, info_b.events_receivers)) {
		// System A is emitting an event that system B is receiving.
		return false;
	}
	if (collides(info_b.events_emitters, info_a.events_receivers)) {
		// System B is emitting an event that system A is receiving.
		return false;
	}

	// TODO Check NOT filter specialization.

	return true;
}

SystemInfo &ECS::get_system_info(godex::system_id p_id) {
	static SystemInfo info;
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, info, "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	return systems_info[p_id];
}

void ECS::get_system_exe_info(godex::system_id p_id, SystemExeInfo &r_info) {
	ERR_FAIL_COND_MSG(verify_system_id(p_id) == false, "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	ERR_FAIL_COND_MSG(systems_info[p_id].exec_info == nullptr, "The System " + systems[p_id] + " is not a standard `System`.");
	systems_info[p_id].exec_info(p_id, r_info);
}

StringName ECS::get_system_name(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, StringName(), "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	return systems[p_id];
}

String ECS::get_system_desc(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, String(), "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	return systems_info[p_id].description;
}

Phase ECS::get_system_phase(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, PHASE_PROCESS, "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	return systems_info[p_id].phase;
}

StringName ECS::get_system_dispatcher(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, StringName(), "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	return systems_info[p_id].dispatcher;
}

int ECS::get_dispatcher_index(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, -1, "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	ERR_FAIL_COND_V_MSG(systems_info[p_id].type != SystemInfo::TYPE_DISPATCHER, -1, "The system " + systems[p_id] + " is not a dispatcher.");
	return systems_info[p_id].dispatcher_index;
}

const LocalVector<SystemDependency> &ECS::get_system_dependencies(godex::system_id p_id) {
	static const LocalVector<SystemDependency> dep;
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, dep, "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	return systems_info[p_id].dependencies;
}

int ECS::get_system_flags(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, Flags::NONE, "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	return systems_info[p_id].flags;
}

bool ECS::is_system_dispatcher(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, false, "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	return systems_info[p_id].type == SystemInfo::TYPE_DISPATCHER;
}

bool ECS::is_temporary_system(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, false, "The TemporarySystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	return systems_info[p_id].type == SystemInfo::TYPE_TEMPORARY;
}

bool ECS::is_dynamic_system(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, false, "The TemporarySystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	return systems_info[p_id].type == SystemInfo::TYPE_DYNAMIC;
}

func_temporary_system_execute ECS::get_func_temporary_system_exe(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, nullptr, "The TemporarySystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	ERR_FAIL_COND_V_MSG(systems_info[p_id].temporary_exec == nullptr, nullptr, "The System : " + systems[p_id] + " is not a TemporarySystem.");
	return systems_info[p_id].temporary_exec;
}

bool ECS::verify_system_id(godex::system_id p_id) {
	return systems.size() > p_id;
}

int ECS::get_dispatchers_count() {
	return dispatcher_count;
}

uint64_t ECS::system_get_size_system_data(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, 0, "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	return systems_info[p_id].system_data_get_size();
}

void ECS::system_new_placement_system_data(godex::system_id p_id, uint8_t *p_mem, Token p_token, World *p_world, Pipeline *p_pipeline) {
	ERR_FAIL_COND_MSG(verify_system_id(p_id) == false, "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	return systems_info[p_id].system_data_new_placement(p_mem, p_token, p_world, p_pipeline, p_id);
}

void ECS::system_delete_placement_system_data(godex::system_id p_id, uint8_t *p_mem) {
	ERR_FAIL_COND_MSG(verify_system_id(p_id) == false, "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	return systems_info[p_id].system_data_delete_placement(p_mem);
}

void ECS::system_set_active_system(godex::system_id p_id, uint8_t *p_mem, bool p_active) {
	ERR_FAIL_COND_MSG(verify_system_id(p_id) == false, "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");
	return systems_info[p_id].system_data_set_active(p_mem, p_active);
}

void ECS::clear_emitters_for_system(godex::system_id p_id) {
	ERR_FAIL_COND_MSG(verify_system_id(p_id) == false, "The SystemID: " + itos(p_id) + " doesn't exists. Are you passing a System ID?");

	SystemExeInfo info;
	ECS::get_system_exe_info(p_id, info);

	// The emitters are defined by the systems that receive the event: for this
	// reason here we are checking the `events_receivers`.
	for (
			OAHashMap<uint32_t, RBSet<String>>::Iterator it = info.events_receivers.iter();
			it.valid;
			it = info.events_receivers.next_iter(it)) {
		if (ECS::verify_event_id(*it.key)) {
			// This is a valid event.
			events_info[*it.key].event_emitters_need_reload = true;
		}
	}
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

	if (active_world) {
		// Deallocate the associated pipelines.
		active_world->release_pipelines();
	}

	WorldECS *prev_node = active_world_node;

	active_world = p_world;
	active_world_node = p_active_world_ecs;
	ready = false;
	active_world_pipeline = nullptr;
	world_token = Token();

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
		// Using crash cond because the user doesn't never use this API directly and
		// at this point the pipeline is always ready.
		CRASH_COND_MSG(p_pipeline->is_ready() == false, "The submitted pipeline is not fully build, you must submit a valid pipeline?.");
	}
#endif
	if (active_world_pipeline) {
		// Deactivate the current pipeline.
		active_world_pipeline->set_active(world_token, false);
	}

	active_world_pipeline = p_pipeline;
	world_token = Token();

	if (active_world_pipeline && active_world) {
		world_token = active_world_pipeline->prepare_world(active_world);
		// Activate this pipeline.
		active_world_pipeline->set_active(world_token, true);
	}
}

Pipeline *ECS::get_active_world_pipeline() const {
	return active_world_pipeline;
}

bool ECS::has_active_world_pipeline() const {
	return active_world_pipeline != nullptr;
}

void ECS::dispatch_active_world() {
	if (likely(world_token.is_valid() && active_world_pipeline)) {
		if (unlikely(ready == false)) {
			// Ready.
			active_world_node->get_tree()->get_root()->propagate_notification(NOTIFICATION_ECS_WORLD_READY);
			ready = true;
		}

		active_world_node->pre_process();

		dispatching = true;
		active_world_pipeline->dispatch(world_token);
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

uint32_t ECS::register_or_get_id_for_component_name(const StringName &p_name) {
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

		// Add a new scripting constant, for fast and easy `component` access.
		ClassDB::bind_integer_constant(get_class_static(), StringName(), String(p_name).replace(".", "_"), id);
		print_line("ComponentScript: " + p_name + " registered with ID: " + itos(id));
	}

	return id;
}

uint32_t ECS::register_or_update_script_component(
		const StringName &p_name,
		const LocalVector<ScriptProperty> &p_properties,
		StorageType p_storage_type,
		Vector<StringName> p_spawners) {
	godex::component_id id = register_or_get_id_for_component_name(p_name);
	DynamicComponentInfo *info;
	ERR_FAIL_COND_V_MSG(components_info[id].dynamic_component_info == nullptr, godex::COMPONENT_NONE, "This component " + p_name + " is not a script component and can't be updated. Your component must be an unique name.");
	info = components_info[id].dynamic_component_info;

	info->property_map.resize(p_properties.size());
	info->properties.resize(p_properties.size());
	info->defaults.resize(p_properties.size());

	// Validate and initialize the parameters.
	for (uint32_t i = 0; i < p_properties.size(); i += 1) {
		if (
				// Filter GDScript file name
				p_properties[i].property.name == p_name
				// Filter C# file name. It uses only the class name
				|| p_properties[i].property.name + ".cs" == p_name) {
			continue;
		}
		// Is  type supported?
		switch (p_properties[i].property.type) {
			case Variant::NIL:
			case Variant::RID:
			case Variant::OBJECT:
			case Variant::SIGNAL:
			case Variant::CALLABLE:
				// TODO what about dictionary and arrays?
				ERR_PRINT("The property " + p_properties[i].property.name + " of script component " + p_name + " is using a pointer variable. This is unsafe, so not supported. Please use a databag.");
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

	return id;
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
