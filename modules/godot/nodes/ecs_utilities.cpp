#include "ecs_utilities.h"

#include "../../../ecs.h"
#include "../../../iterators/dynamic_query.h"
#include "../../../systems/dynamic_system.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "core/object/script_language.h"
#include "editor/editor_node.h"
#include "entity.h"
#include "shared_component_resource.h"

void System::_bind_methods() {
	ClassDB::bind_method(D_METHOD("execute_in", "phase", "dispatcher"), &System::execute_in, DEFVAL(godex::SYSTEM_NONE));
	ClassDB::bind_method(D_METHOD("execute_after", "system_name"), &System::execute_after);
	ClassDB::bind_method(D_METHOD("execute_before", "system_name"), &System::execute_before);

	ClassDB::bind_method(D_METHOD("with_query", "query"), &System::with_query_gd);
	ClassDB::bind_method(D_METHOD("with_databag", "databag_id", "mutability"), &System::with_databag);
	ClassDB::bind_method(D_METHOD("with_storage", "component_id"), &System::with_storage);
	ClassDB::bind_method(D_METHOD("with_events_emitter", "event_id"), &System::with_events_emitter);
	ClassDB::bind_method(D_METHOD("with_events_receiver", "event_id", "emitter_name"), &System::with_events_receiver);

	ClassDB::bind_method(D_METHOD("get_system_id"), &System::get_system_id);

	BIND_ENUM_CONSTANT(IMMUTABLE);
	BIND_ENUM_CONSTANT(MUTABLE);

	ClassDB::add_virtual_method(get_class_static(), MethodInfo("_prepare"));
	// TODO how to define `_execute`? It has  dynamic argument, depending on the `_prepare` function.
}

const String &System::get_script_path() const {
	return script_path;
}

System::System() {
}

System::~System() {}

void System::execute_in(Phase p_phase, uint32_t p_dispatcher_id) {
	ERR_FAIL_COND_MSG(info == nullptr, "No info set. This function can be called only within the `_prepare`.");
	if (p_dispatcher_id != godex::SYSTEM_NONE) {
		const StringName name = ECS::get_system_name(p_dispatcher_id);
		ERR_FAIL_COND(name == StringName());
		info->execute_in(p_phase, name);
	} else {
		info->execute_in(p_phase);
	}
}

void System::execute_after(uint32_t p_system) {
	ERR_FAIL_COND_MSG(info == nullptr, "No info set. This function can be called only within the `_prepare`.");
	const StringName name = ECS::get_system_name(p_system);
	ERR_FAIL_COND(name == StringName());
	info->execute_after(name);
}

void System::execute_before(uint32_t p_system) {
	ERR_FAIL_COND_MSG(info == nullptr, "No info set. This function can be called only within the `_prepare`.");
	const StringName name = ECS::get_system_name(p_system);
	ERR_FAIL_COND(name == StringName());
	info->execute_before(name);
}

void System::with_query_gd(Object *p_query) {
	with_query(Object::cast_to<godex::DynamicQuery>(p_query));
}

void System::with_query(godex::DynamicQuery *p_query) {
	ERR_FAIL_COND_MSG(info == nullptr, "No info set. This function can be called only within the `_prepare`.");
	info->with_query(p_query);
}

void System::with_databag(uint32_t p_databag_id, Mutability p_mutability) {
	ERR_FAIL_COND_MSG(info == nullptr, "No info set. This function can be called only within the `_prepare`.");
	info->with_databag(p_databag_id, p_mutability == MUTABLE);
}

void System::with_storage(uint32_t p_component_id) {
	ERR_FAIL_COND_MSG(info == nullptr, "No info set. This function can be called only within the `_prepare`.");
	info->with_storage(p_component_id);
}

void System::with_events_emitter(godex::event_id p_event_id) {
	ERR_FAIL_COND_MSG(info == nullptr, "No info set. This function can be called only within the `_prepare`.");
	info->with_events_emitter(p_event_id);
}

void System::with_events_receiver(godex::event_id p_event_id, const String &p_emitter_name) {
	ERR_FAIL_COND_MSG(info == nullptr, "No info set. This function can be called only within the `_prepare`.");
	info->with_events_receiver(p_event_id, p_emitter_name);
}

godex::system_id System::get_system_id() const {
	return id;
}

void System::prepare(godex::DynamicSystemExecutionData *p_info) {
	ERR_FAIL_COND_MSG(p_info == nullptr, "[FATAL] This is not supposed to happen.");
	ERR_FAIL_COND_MSG(get_script_instance() == nullptr, "[FATAL] This is not supposed to happen.");

	info = p_info;
	info->set_system_id(id);

	Callable::CallError err;
	get_script_instance()->callp("_prepare", nullptr, 0, err);

	info = nullptr;
}

String System::validate_script(Ref<Script> p_script) {
	if (p_script.is_null()) {
		return TTR("Script is null.");
	}
	if (p_script->is_valid() == false) {
		return TTR("Script has some errors.");
	}
	if ("System" != p_script->get_instance_base_type()) {
		return TTR("This script is not extending `System`.");
	}
	List<MethodInfo> methods;
	p_script->get_script_method_list(&methods);
	bool has_prepare = false;
	bool has_execute = false;
	for (List<MethodInfo>::Element *e = methods.front(); e; e = e->next()) {
		if (e->get().name == "_prepare") {
			has_prepare = true;
		}
		if (e->get().name == "_execute") {
			has_execute = true;
			// TODO consider add input check, so to notify the user if the system is not valid.
		}
	}
	if (has_prepare == false) {
		return TTR("This script is not overriding the function `_prepare()`.");
	}
	if (has_execute == false) {
		return TTR("This script is not overriding the function `_execute(...)`.");
	}

	List<PropertyInfo> properties;
	p_script->get_script_property_list(&properties);
	for (List<PropertyInfo>::Element *e = properties.front(); e; e = e->next()) {
		if (e->get().name == p_script->get_class_category().name) {
			properties.erase(e);
		}
	}

	if (properties.size()) {
		return TTR("The System script can't have any property in it. It possible to only access `Component`s and `Databag`s.");
	}

	// This script is safe to use.
	return "";
}

void System::get_system_exec_info(godex::system_id p_id, SystemExeInfo &r_info) {
	Ref<System> system = ScriptEcs::get_singleton()->get_script_system(ECS::get_system_name(p_id));
	r_info.valid = false;
	ERR_FAIL_COND_MSG(!system.is_valid(), "The `ScriptEcs` wasn't able to create the scripted system: `" + ECS::get_system_name(p_id) + "`");

	godex::DynamicSystemExecutionData dynamic_system_data;
	system->prepare(&dynamic_system_data);

	godex::DynamicSystemExecutionData::get_info(dynamic_system_data, r_info);
}

uint64_t System::dynamic_system_data_get_size() {
	return sizeof(godex::DynamicSystemExecutionData);
}

void System::dynamic_system_data_new_placement(uint8_t *r_mem, Token p_token, World *p_world, Pipeline *p_pipeline, godex::system_id p_id) {
	godex::DynamicSystemExecutionData *dynamic_system_data = new (r_mem) godex::DynamicSystemExecutionData();
	dynamic_system_data->set_system_id(p_id);

	Ref<System> system = ScriptEcs::get_singleton()->get_script_system(ECS::get_system_name(p_id));
	ERR_FAIL_COND_MSG(system.is_valid() == false, "The scripted system: `" + ECS::get_system_name(p_id) + "` is invalid, can't created the SystemExecutionData properly.");

	system->prepare(dynamic_system_data);
	// Set this object as target.
	dynamic_system_data->set_target(system->get_script_instance());
	// Build.
	dynamic_system_data->prepare_world(p_world);
}

void System::dynamic_system_data_delete_placement(uint8_t *p_mem) {
	((godex::DynamicSystemExecutionData *)p_mem)->~DynamicSystemExecutionData();
}

void System::dynamic_system_data_set_active(uint8_t *p_mem, bool p_active) {
	godex::DynamicSystemExecutionData *system_exec_data = (godex::DynamicSystemExecutionData *)p_mem;
	system_exec_data->set_active(p_active);
}

void SystemBundle::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add", "system_id"), &SystemBundle::add);
	ClassDB::bind_method(D_METHOD("with_description", "desc"), &SystemBundle::with_description);
	ClassDB::bind_method(D_METHOD("execute_before", "system_id"), &SystemBundle::execute_before);
	ClassDB::bind_method(D_METHOD("execute_after", "system_id"), &SystemBundle::execute_after);

	ClassDB::add_virtual_method(get_class_static(), MethodInfo("_prepare"));
}

void SystemBundle::__prepare() {
	ERR_FAIL_COND_MSG(get_script_instance() == nullptr, "[FATAL] This is not supposed to happen.");
	ERR_FAIL_COND_MSG(name == StringName(), "[FATAL] The name of this bundle is not set.");

	// Register the system bundle.
	ECS::register_system_bundle(name);

	Callable::CallError err;
	get_script_instance()->callp("_prepare", nullptr, 0, err);
}

const String &SystemBundle::get_script_path() const {
	return script_path;
}

void SystemBundle::add(uint32_t p_system_id) {
	ERR_FAIL_COND_MSG(name == StringName(), "Never call `_prepare` directly. Use `__fetch_descriptor` instead.");
	const StringName system_name = ECS::get_system_name(p_system_id);
	ERR_FAIL_COND_MSG(system_name == StringName(), "The system id `" + itos(p_system_id) + "` is not associated with any system.");
	ECS::get_system_bundle(ECS::get_system_bundle_id(name)).add(system_name);
}

void SystemBundle::with_description(const String &p_desc) {
	ERR_FAIL_COND_MSG(name == StringName(), "Never call `_prepare` directly. Use `__fetch_descriptor` instead.");
	ECS::get_system_bundle(ECS::get_system_bundle_id(name)).set_description(p_desc);
}

void SystemBundle::execute_before(uint32_t p_system_id) {
	ERR_FAIL_COND_MSG(name == StringName(), "Never call `_prepare` directly. Use `__fetch_descriptor` instead.");
	const StringName system_name = ECS::get_system_name(p_system_id);
	ERR_FAIL_COND_MSG(system_name == StringName(), "The system id `" + itos(p_system_id) + "` is not associated with any system.");
	ECS::get_system_bundle(ECS::get_system_bundle_id(name)).before(system_name);
}

void SystemBundle::execute_after(uint32_t p_system_id) {
	ERR_FAIL_COND_MSG(name == StringName(), "Never call `_prepare` directly. Use `__fetch_descriptor` instead.");
	const StringName system_name = ECS::get_system_name(p_system_id);
	ERR_FAIL_COND_MSG(system_name == StringName(), "The system id `" + itos(p_system_id) + "` is not associated with any system.");
	ECS::get_system_bundle(ECS::get_system_bundle_id(name)).after(system_name);
}

String SystemBundle::validate_script(Ref<Script> p_script) {
	if (p_script.is_null()) {
		return TTR("Script is null.");
	}
	if (p_script->is_valid() == false) {
		return TTR("Script has some errors.");
	}
	if ("SystemBundle" != p_script->get_instance_base_type()) {
		return TTR("This script is not extending `SystemBundle`.");
	}
	List<MethodInfo> methods;
	p_script->get_script_method_list(&methods);
	bool has_prepare = false;
	for (List<MethodInfo>::Element *e = methods.front(); e; e = e->next()) {
		if (e->get().name == "_prepare") {
			has_prepare = true;
			break;
		}
	}
	if (has_prepare == false) {
		return TTR("This script is not overriding the function `_prepare()`.");
	}
	// This script is safe to use.
	return "";
}

void Component::_bind_methods() {}

const String &Component::get_script_path() const {
	return script_path;
}

Component::Component() {}

Component::~Component() {}

void Component::internal_set_name(StringName p_name) {
	name = p_name;
}

StringName Component::get_name() const {
	return name;
}

void Component::get_component_property_list(List<PropertyInfo> *r_info) {
	Ref<Script> script = get_script();
	if (script.is_null()) {
		return;
	}

	script->get_script_property_list(r_info);
}

Variant Component::get_property_default_value(StringName p_property_name) {
	bool valid = false;
	const Variant ret = get(p_property_name, &valid);
	return valid ? ret : Variant();
}

Vector<StringName> Component::get_spawners() {
	Vector<StringName> spawners;
	if (has_method("get_spawners")) {
		spawners = call("get_spawners");
	}
	return spawners;
}

String Component::validate_script(Ref<Script> p_script) {
	if (p_script.is_null()) {
		return TTR("Script is null.");
	}
	if (p_script->is_valid() == false) {
		return TTR("Script has some errors.");
	}
	if ("Component" != p_script->get_instance_base_type()) {
		return TTR("This script is not extending `Component`.");
	}

	// Make sure doesn't have any function in it.
	List<MethodInfo> methods;
	p_script->get_script_method_list(&methods);

	// Make sure this Component has the correct methods.
	for (int i = 0; i < methods.size(); i += 1) {
		// Only this method is allowed.
		if (methods[i].name != "@implicit_new" && methods[i].name != "get_spawners") {
			return TTR("The only method the Component can have is the `get_spawners()`.");
		}
	}

	List<PropertyInfo> properties;
	p_script->get_script_property_list(&properties);
	for (List<PropertyInfo>::Element *e = properties.front(); e; e = e->next()) {
		if (
				// Filter GDScript file name
				e->get().name == p_script->get_class_category().name
				// Filter C# file name. It uses only the class name
				|| p_script->get_path().ends_with(e->get().name + ".cs")) {
			continue;
		}
		switch (e->get().type) {
			case Variant::NIL:
				return "(" + e->get().name + ") " + TTR("Please make sure all variables are typed.");
			case Variant::RID:
			case Variant::OBJECT:
				return "(" + e->get().name + ") " + TTR("The Component can't hold unsafe references. The same reference could be holded by multiple things into the engine, this invalidates the thread safety of the ECS model. Please use a Databag or report your use case so a safe native type will be provided instead.");
			case Variant::SIGNAL:
			case Variant::CALLABLE:
				return "(" + e->get().name + ") " + TTR("The Component can't hold signals or callables. Please report your use case.");
			default:
				// Nothing to worry about.
				break;
		}
	}

	// This script is safe to use.
	return "";
}

String databag_validate_script(Ref<Script> p_script) {
	ERR_FAIL_COND_V(p_script.is_null(), "Script is null.");
	ERR_FAIL_COND_V(p_script->is_valid() == false, "Script has some errors.");
	ERR_FAIL_COND_V("Databag" != p_script->get_instance_base_type(), "This script is not extending `Databag`.");

	// TODO the databag are special. Make sure we can use as databag Objects,
	//      loaded files, anything. So we can easily use static things loaded from the disk.

	// This script is safe to use.
	return "Not yet implemented";
}

ComponentDepot::~ComponentDepot() {
}

StaticComponentDepot::~StaticComponentDepot() {
	if (component != nullptr) {
		ECS::free_component(component_id, component);
		component = nullptr;
	}
}

void StaticComponentDepot::init(const StringName &p_name) {
	ERR_FAIL_COND_MSG(component_name != StringName(), "The component is already initialized.");
	const godex::component_id id = ECS::get_component_id(p_name);
	ERR_FAIL_COND_MSG(id == godex::COMPONENT_NONE, "This component " + p_name + " doesn't exist.");
	component_name = p_name;

	component = ECS::new_component(id);
	component_id = id;
}

bool StaticComponentDepot::_setv(const StringName &p_name, const Variant &p_value) {
	ERR_FAIL_COND_V_MSG(component == nullptr, false, "This depot is not initialized.");
	return ECS::unsafe_component_set_by_name(component_id, component, p_name, p_value);
}

bool StaticComponentDepot::_getv(const StringName &p_name, Variant &r_ret) const {
	ERR_FAIL_COND_V_MSG(component == nullptr, false, "This depot is not initialized.");
	return ECS::unsafe_component_get_by_name(component_id, component, p_name, r_ret);
}

Dictionary StaticComponentDepot::get_properties_data() const {
	Dictionary d;

	ERR_FAIL_COND_V_MSG(component == nullptr, d, "This depot is not initialized.");

	List<PropertyInfo> props;
	ECS::unsafe_component_get_property_list(component_id, component, &props);
	for (PropertyInfo &prop : props) {
		Variant v;
		const StringName prop_name = prop.name;
		ECS::unsafe_component_get_by_name(component_id, component, prop_name, v);
		d[prop_name] = v;
	}

	return d;
}

void StaticComponentDepot::_get_property_list(List<PropertyInfo> *r_list) const {
	ERR_FAIL_COND_MSG(component_id == godex::COMPONENT_NONE, "The component is not initialized.");
	ECS::unsafe_component_get_property_list(
			component_id,
			component,
			r_list);
}

void ScriptComponentDepot::init(const StringName &p_name) {
	ERR_FAIL_COND_MSG(component_name != StringName(), "The component is already initialized.");
	ERR_FAIL_COND_MSG(ScriptEcs::get_singleton()->get_script_component(p_name).is_valid() == false, "Thid component " + p_name + " is not a script component.");
	component_name = p_name;
}

bool ScriptComponentDepot::_setv(const StringName &p_name, const Variant &p_value) {
	ERR_FAIL_COND_V_MSG(component_name == StringName(), false, "The component is not initialized.");
	data[p_name] = p_value.duplicate(true);
	return true;
}

bool ScriptComponentDepot::_getv(const StringName &p_name, Variant &r_ret) const {
	ERR_FAIL_COND_V_MSG(component_name == StringName(), false, "The component is not initialized.");
	const Variant *v = data.getptr(p_name);
	if (v == nullptr) {
		// Take the default.
		const godex::component_id id = ECS::get_component_id(component_name);
		ERR_FAIL_COND_V_MSG(id == godex::COMPONENT_NONE, false, "[FATAL] The component `" + component_name + "` doesn't exist. This is not supposed to happen.");
		r_ret = ECS::get_component_property_default(id, p_name);
		return true;
	} else {
		r_ret = v->duplicate(true);
		return true;
	}
}

Dictionary ScriptComponentDepot::get_properties_data() const {
	return data;
}

void ScriptComponentDepot::_get_property_list(List<PropertyInfo> *r_list) const {
	ERR_FAIL_COND_MSG(component_name == StringName(), "The component is not initialized.");
	ECS::unsafe_component_get_property_list(
			ECS::get_component_id(component_name),
			// ScriptedComponents doesn't support dynamic set/get yet.
			nullptr,
			r_list);
}

void SharedComponentDepot::init(const StringName &p_name) {
	ERR_FAIL_COND_MSG(component_name != StringName(), "The component is already initialized.");
	ERR_FAIL_COND_MSG(ECS::is_component_sharable(ECS::get_component_id(p_name)) == false, "Thid component " + p_name + " is not a shared component.");
	component_name = p_name;
}

bool SharedComponentDepot::_setv(const StringName &p_name, const Variant &p_value) {
	ERR_FAIL_COND_V_MSG(component_name == StringName(), false, "The component is not initialized.");

	Ref<SharedComponentResource> shared = p_value;
	if (shared.is_valid()) {
		// Validate the shared component.
		if (shared->is_init()) {
			ERR_FAIL_COND_V_MSG(shared->get_component_name() != component_name, false, "The passed component is of type: " + shared->get_component_name() + " while the expected one is of type: " + component_name + ".");
		} else {
			// This component is new and not even init, so do it now.
			shared->init(component_name);
		}
		data = shared;
		return true;
	} else {
		// Try to set the value inside the shared component instead

		if (data.is_null()) {
			// The shared component is still null, so create it.
			data.instantiate();
			data->init(component_name);
		}

		bool success = false;
		data->set(p_name, p_value, &success);
		return success;
	}
}

bool SharedComponentDepot::_getv(const StringName &p_name, Variant &r_ret) const {
	if (p_name == "resource") {
		r_ret = data;
		return true;
	} else {
		ERR_FAIL_COND_V_MSG(data.is_null(), false, "The component is not initialized.");
		bool success = false;
		r_ret = data->get(p_name, &success);
		if (success == false) {
			// Take the default
			const godex::component_id id = ECS::get_component_id(p_name);
			ERR_FAIL_COND_V(id == godex::COMPONENT_NONE, false);
			r_ret = ECS::get_component_property_default(id, p_name);
			return true;
		} else {
			return true;
		}
	}
	return false;
}

Dictionary SharedComponentDepot::get_properties_data() const {
	Dictionary d;
	d[SNAME("resource")] = data;
	return d;
}

void SharedComponentDepot::_get_property_list(List<PropertyInfo> *r_list) const {
	r_list->push_back(PropertyInfo(Variant::OBJECT, "resource", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}
