
#include "ecs.h"

#include "components/dynamic_component.h"
#include "core/object/message_queue.h"
#include "pipeline/pipeline.h"
#include "scene/main/scene_tree.h"
#include "systems/dynamic_system.h"
#include "world/world.h"

ECS *ECS::singleton = nullptr;
LocalVector<StringName> ECS::components;
LocalVector<ComponentInfo> ECS::components_info;
LocalVector<StringName> ECS::resources;
LocalVector<ResourceInfo> ECS::resources_info;
LocalVector<StringName> ECS::systems;
LocalVector<SystemInfo> ECS::systems_info;

void ECS::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_active_world"), &ECS::get_active_world_gds);

	ClassDB::bind_method(D_METHOD("get_component_id", "name"), &ECS::get_component_id_obj);
	ClassDB::bind_method(D_METHOD("verify_component_id", "name"), &ECS::verify_component_id_obj);

	ClassDB::bind_method(D_METHOD("get_resource_id", "name"), &ECS::get_resource_id_obj);
	ClassDB::bind_method(D_METHOD("verify_resource_id", "name"), &ECS::verify_resource_id_obj);

	ClassDB::bind_method(D_METHOD("get_system_id", "name"), &ECS::get_system_id_obj);
	ClassDB::bind_method(D_METHOD("verify_system_id", "name"), &ECS::verify_system_id_obj);

	ADD_SIGNAL(MethodInfo("world_loaded"));
	ADD_SIGNAL(MethodInfo("world_pre_unload"));
	ADD_SIGNAL(MethodInfo("world_unloaded"));
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
	ERR_FAIL_INDEX_V_MSG(p_component_id, components.size(), "", "The `component_id` is invalid: " + itos(p_component_id));
	return components[p_component_id];
}

const LocalVector<PropertyInfo> *ECS::get_component_properties(uint32_t p_component_id) {
	ERR_FAIL_INDEX_V_MSG(p_component_id, components.size(), nullptr, "The `component_id` is invalid: " + itos(p_component_id));
	if (components_info[p_component_id].dynamic_component_info != nullptr) {
		return components_info[p_component_id].dynamic_component_info->get_properties();
	} else {
		return components_info[p_component_id].get_properties();
	}
}

Variant ECS::get_component_property_default(uint32_t p_component_id, StringName p_property_name) {
	ERR_FAIL_COND_V_MSG(verify_component_id(p_component_id) == false, Variant(), "The component " + itos(p_component_id) + " is invalid.");

	if (components_info[p_component_id].dynamic_component_info != nullptr) {
		// Script
		return components_info[p_component_id].dynamic_component_info->get_property_default(p_property_name);
	} else {
		// Native
		return components_info[p_component_id].get_property_default(p_property_name);
	}
}

bool ECS::verify_resource_id(godex::resource_id p_id) {
	return p_id < resources.size();
}

godex::Resource *ECS::create_resource(godex::resource_id p_id) {
#ifdef DEBUG_ENABLED
	// Crash cond because this function is not supposed to fail in any way.
	CRASH_COND_MSG(ECS::verify_resource_id(p_id) == false, "This resource id " + itos(p_id) + " is not valid.");
#endif
	return resources_info[p_id].create_resource();
}

uint32_t ECS::get_resource_count() {
	return resources.size();
}

uint32_t ECS::get_resource_id(const StringName &p_name) {
	const int64_t id = resources.find(p_name);
	return id >= 0 ? godex::resource_id(id) : UINT32_MAX;
}

StringName ECS::get_resource_name(godex::resource_id p_resource_id) {
	ERR_FAIL_INDEX_V_MSG(p_resource_id, resources.size(), "", "The `resource_id` is invalid: " + itos(p_resource_id));
	return resources[p_resource_id];
}

// Undefine the macro defined into `ecs.h` so we can define the method properly.
#undef register_system
void ECS::register_system(func_get_system_exe_info p_func_get_exe_info, StringName p_name, String p_description) {
	{
		const uint32_t id = get_system_id(p_name);
		ERR_FAIL_COND_MSG(id != UINT32_MAX, "The system is already registered.");
	}

	const godex::system_id id = systems.size();
	systems.push_back(p_name);
	systems_info.push_back({ p_description,
			UINT32_MAX,
			p_func_get_exe_info });

	print_line("System: " + p_name + " registered with ID: " + itos(id));
}

godex::system_id ECS::register_dynamic_system(StringName p_name, const String &p_description) {
	{
		const uint32_t id = get_system_id(p_name);
		ERR_FAIL_COND_V_MSG(id != UINT32_MAX, UINT32_MAX, "The system is already registered.");
	}

	const godex::system_id id = systems.size();

	// Used to assign a static function to this dynamic system, check the
	// DynamicSystem doc to know more (../systems/dynamic_system.h).
	const uint32_t dynamic_system_id = godex::register_dynamic_system();

	systems.push_back(p_name);
	systems_info.push_back({ p_description,
			dynamic_system_id,
			godex::get_func_dynamic_system_exec_info(dynamic_system_id) });

	godex::get_dynamic_system_info(dynamic_system_id)->set_system_id(id);

	print_line("Dynamic system: " + p_name + " registered with ID: " + itos(id));

	return id;
}

godex::system_id ECS::get_system_id(const StringName &p_name) {
	const int64_t index = systems.find(p_name);
	return index >= 0 ? godex::system_id(index) : UINT32_MAX;
}

uint32_t ECS::get_systems_count() {
	return systems.size();
}

func_get_system_exe_info ECS::get_func_system_exe_info(godex::system_id p_id) {
	ERR_FAIL_INDEX_V_MSG(p_id, systems_info.size(), nullptr, "The SystemID: " + itos(p_id) + " doesn't exists.");
	return systems_info[p_id].exec_info;
}

void ECS::get_system_exe_info(godex::system_id p_id, SystemExeInfo &r_info) {
	ERR_FAIL_INDEX_MSG(p_id, systems_info.size(), "The SystemID: " + itos(p_id) + " doesn't exists.");
	return systems_info[p_id].exec_info(r_info);
}

StringName ECS::get_system_name(godex::system_id p_id) {
	ERR_FAIL_INDEX_V_MSG(p_id, systems_info.size(), "", "The SystemID: " + itos(p_id) + " doesn't exists.");
	return systems[p_id];
}

String ECS::get_system_desc(godex::system_id p_id) {
	ERR_FAIL_INDEX_V_MSG(p_id, systems_info.size(), "", "The SystemID: " + itos(p_id) + " doesn't exists.");
	return systems_info[p_id].description;
}

void ECS::set_dynamic_system_target(godex::system_id p_id, ScriptInstance *p_target) {
	ERR_FAIL_COND_MSG(verify_system_id(p_id) == false, "This system " + itos(p_id) + " doesn't exists.");
	ERR_FAIL_COND_MSG(systems_info[p_id].dynamic_system_id == UINT32_MAX, "The system " + itos(p_id) + " is not a dynamic system.");
	godex::DynamicSystemInfo *info = godex::get_dynamic_system_info(systems_info[p_id].dynamic_system_id);
	info->set_target(p_target);
}

godex::DynamicSystemInfo *ECS::get_dynamic_system_info(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, nullptr, "This system " + itos(p_id) + " doesn't exists.");
	ERR_FAIL_COND_V_MSG(systems_info[p_id].dynamic_system_id == UINT32_MAX, nullptr, "The system " + itos(p_id) + " is not a dynamic system.");
	return godex::get_dynamic_system_info(systems_info[p_id].dynamic_system_id);
}

bool ECS::is_system_dispatcher(godex::system_id p_id) {
	ERR_FAIL_COND_V_MSG(verify_system_id(p_id) == false, false, "This system " + itos(p_id) + " doesn't exists.");
	if (systems_info[p_id].dynamic_system_id == UINT32_MAX) {
		// Only dynamic systems are pipeline_dispatchers.
		return false;
	}
	godex::DynamicSystemInfo *info = godex::get_dynamic_system_info(systems_info[p_id].dynamic_system_id);
	return info->is_system_dispatcher();
}

void ECS::set_system_pipeline(godex::system_id p_id, Pipeline *p_pipeline) {
	ERR_FAIL_COND_MSG(verify_system_id(p_id) == false, "This system " + itos(p_id) + " doesn't exists.");
	ERR_FAIL_COND_MSG(systems_info[p_id].dynamic_system_id == UINT32_MAX, "The system " + itos(p_id) + " is not a dynamic system.");
	godex::DynamicSystemInfo *info = godex::get_dynamic_system_info(systems_info[p_id].dynamic_system_id);
	ERR_FAIL_COND_MSG(info->is_system_dispatcher() == false, "The system " + itos(p_id) + " is not a sub pipeline dispatcher.");
	info->set_pipeline(p_pipeline);
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

void ECS::set_active_world(World *p_world) {
	if (active_world != nullptr) {
		if (p_world == nullptr) {
			emit_signal("world_pre_unload");
		} else {
			ERR_FAIL_COND("Before adding a new world it's necessary remove the current one by calling `set_active_world(nullptr);`.");
		}
	}

	active_world = p_world;
	active_world_pipeline = nullptr;

	if (active_world != nullptr) {
		// The world is just loaded.
		emit_signal("world_loaded");
	} else {
		// The world is just unloaded.
		emit_signal("world_unloaded");
	}
}

World *ECS::get_active_world() const {
	return active_world;
}

godex::AccessResource *ECS::get_active_world_gds() {
	ERR_FAIL_COND_V_MSG(active_world == nullptr, nullptr, "No active world at the moment.");
	world_access.__resource = active_world;
	world_access.__mut = true;
	return &world_access;
}

bool ECS::has_active_world() const {
	return active_world != nullptr;
}

void ECS::set_active_world_pipeline(Pipeline *p_pipeline) {
#ifdef DEBUG_ENABLED
	if (p_pipeline) {
		// Using crash cond because the user doesn't never use this API directly.
		CRASH_COND_MSG(p_pipeline->is_ready() == false, "The submitted pipeline is not fully build.");
	}
#endif
	active_world_pipeline = p_pipeline;
}

Pipeline *ECS::get_active_world_pipeline() const {
	return active_world_pipeline;
}

bool ECS::has_active_world_pipeline() const {
	return active_world_pipeline != nullptr;
}

void ECS::dispatch_active_world() {
	if (likely(active_world && active_world_pipeline)) {
		dispatching = true;
		active_world_pipeline->dispatch(active_world);
		dispatching = false;
	}
}

void ECS::ecs_init() {
}

uint32_t ECS::register_script_component(StringName p_name, const LocalVector<ScriptProperty> &p_properties, StorageType p_storage_type) {
	{
		const uint32_t id = get_component_id(p_name);
		ERR_FAIL_COND_V_MSG(id != UINT32_MAX, UINT32_MAX, "The script component " + p_name + " is already registered.");
	}

	// This component is not registered, go ahead.
	DynamicComponentInfo *info = memnew(DynamicComponentInfo);

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
				memdelete(info);
				ERR_PRINT("The script component " + p_name + " is using a pointer variable. This is unsafe, so not supported. Please use a resource.");
				return UINT32_MAX;
			default:
				// Valid!
				break;
		}

		// Is default type correct?
		if (p_properties[i].property.type != p_properties[i].default_value.get_type()) {
			memdelete(info);
			ERR_PRINT("The script variable " + p_name + "::" + p_properties[i].property.name + " is being initialized with a different default variable type.");
			return UINT32_MAX;
		}

		info->property_map.insert(p_properties[i].property.name, i);
		info->properties[i] = p_properties[i].property;
		info->defaults[i] = p_properties[i].default_value;
	}

	info->component_id = components.size();
	info->storage_type = p_storage_type;

	components.push_back(p_name);
	components_info.push_back(
			ComponentInfo{
					nullptr,
					nullptr,
					nullptr,
					info });

	return info->component_id;
}

bool ECS::verify_component_id(uint32_t p_component_id) {
	return components.size() > p_component_id;
}

Storage *ECS::create_storage(uint32_t p_component_id) {
#ifdef DEBUG_ENABLED
	// Crash cond because this function is not supposed to fail in any way.
	CRASH_COND_MSG(ECS::verify_component_id(p_component_id) == false, "This component id " + itos(p_component_id) + " is not valid.");
#endif
	if (components_info[p_component_id].dynamic_component_info) {
		// This is a script component
		return components_info[p_component_id].dynamic_component_info->create_storage();
	} else {
		// This is a native component.
		return components_info[p_component_id].create_storage();
	}
}
