#include "ecs_utilities.h"

#include "../../../ecs.h"
#include "../../../systems/dynamic_system.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "core/object/script_language.h"
#include "editor/editor_node.h"
#include "entity.h"
#include "shared_component_resource.h"

void System::_bind_methods() {
	ClassDB::bind_method(D_METHOD("execute_in_phase", "phase"), &System::execute_in_phase);
	ClassDB::bind_method(D_METHOD("execute_after", "system_name"), &System::execute_after);
	ClassDB::bind_method(D_METHOD("execute_before", "system_name"), &System::execute_before);

	ClassDB::bind_method(D_METHOD("set_space", "space"), &System::set_space);
	ClassDB::bind_method(D_METHOD("with_databag", "databag_id", "mutability"), &System::with_databag);
	ClassDB::bind_method(D_METHOD("with_storage", "component_id"), &System::with_storage);
	ClassDB::bind_method(D_METHOD("with_component", "component_id", "mutability"), &System::with_component);
	ClassDB::bind_method(D_METHOD("maybe_component", "component_id", "mutability"), &System::maybe_component);
	ClassDB::bind_method(D_METHOD("changed_component", "component_id", "mutability"), &System::changed_component);
	ClassDB::bind_method(D_METHOD("not_component", "component_id"), &System::not_component);

	ClassDB::bind_method(D_METHOD("get_current_entity_id"), &System::get_current_entity_id);

	ClassDB::bind_method(D_METHOD("get_system_id"), &System::get_system_id);

	BIND_ENUM_CONSTANT(IMMUTABLE);
	BIND_ENUM_CONSTANT(MUTABLE);

	ClassDB::add_virtual_method(get_class_static(), MethodInfo("_prepare"));
	// TODO how to define `_for_each`? It has  dynamic argument, depending on the `_prepare` function.
}

const String &System::get_script_path() const {
	return script_path;
}

System::System() {
}

System::~System() {
	if (id != UINT32_MAX) {
		ECS::set_dynamic_system_target(id, nullptr);
	}
}

void System::execute_in_phase(Phase p_phase) {
	if (system_descriptor != nullptr) {
		system_descriptor->phase = p_phase;
	}
}

void System::execute_after(const StringName &p_system_name) {
	if (system_descriptor != nullptr) {
		system_descriptor->dependencies.push_back({ false, p_system_name });
	}
}

void System::execute_before(const StringName &p_system_name) {
	if (system_descriptor != nullptr) {
		system_descriptor->dependencies.push_back({ true, p_system_name });
	}
}

void System::set_space(Space p_space) {
	if (system_descriptor != nullptr) {
		// Nothing to do.
		return;
	}
	ERR_FAIL_COND_MSG(prepare_in_progress == false, "No info set. This function can be called only within the `_prepare`.");
	info->set_space(p_space);
}

void System::with_databag(uint32_t p_databag_id, Mutability p_mutability) {
	if (system_descriptor != nullptr) {
		//system_descriptor->;
	} else {
		ERR_FAIL_COND_MSG(prepare_in_progress == false, "No info set. This function can be called only within the `_prepare`.");
		info->with_databag(p_databag_id, p_mutability == MUTABLE);
	}
}

void System::with_storage(uint32_t p_component_id) {
	if (system_descriptor != nullptr) {
		// Nothing to do.
		return;
	}
	ERR_FAIL_COND_MSG(prepare_in_progress == false, "No info set. This function can be called only within the `_prepare`.");
	info->with_storage(p_component_id);
}

void System::with_component(uint32_t p_component_id, Mutability p_mutability) {
	if (system_descriptor != nullptr) {
		// Nothing to do.
		return;
	}
	ERR_FAIL_COND_MSG(prepare_in_progress == false, "No info set. This function can be called only within the `_prepare`.");
	info->with_component(p_component_id, p_mutability == MUTABLE);
}

void System::maybe_component(uint32_t p_component_id, Mutability p_mutability) {
	if (system_descriptor != nullptr) {
		// Nothing to do.
		return;
	}
	ERR_FAIL_COND_MSG(prepare_in_progress == false, "No info set. This function can be called only within the `_prepare`.");
	info->maybe_component(p_component_id, p_mutability == MUTABLE);
}

void System::changed_component(uint32_t p_component_id, Mutability p_mutability) {
	if (system_descriptor != nullptr) {
		// Nothing to do.
		return;
	}
	ERR_FAIL_COND_MSG(prepare_in_progress == false, "No info set. This function can be called only within the `_prepare`.");
	info->changed_component(p_component_id, p_mutability == MUTABLE);
}

void System::not_component(uint32_t p_component_id) {
	if (system_descriptor != nullptr) {
		// Nothing to do.
		return;
	}
	ERR_FAIL_COND_MSG(prepare_in_progress == false, "No info set. This function can be called only within the `_prepare`.");
	info->not_component(p_component_id);
}

godex::system_id System::get_system_id() const {
	return id;
}

uint32_t System::get_current_entity_id() const {
	ERR_FAIL_COND_V_MSG(info == nullptr, UINT32_MAX, "This systems doesn't seems ready.");
	return info->get_current_entity_id();
}

void System::__force_set_system_info(godex::DynamicSystemInfo *p_info, godex::system_id p_id) {
	id = p_id;
	info = p_info;
}

void System::prepare(godex::DynamicSystemInfo *p_info, godex::system_id p_id) {
	ERR_FAIL_COND_MSG(p_info == nullptr, "[FATAL] This is not supposed to happen.");
	ERR_FAIL_COND_MSG(get_script_instance() == nullptr, "[FATAL] This is not supposed to happen.");

	// Set the components and databags
	id = p_id;
	info = p_info;
	Callable::CallError err;
	prepare_in_progress = true;
	get_script_instance()->call("_prepare", nullptr, 0, err);
	prepare_in_progress = false;

	// Set this object as target.
	info->set_target(get_script_instance());
	info->build();
}

void System::__fetch_descriptor(SystmeDescriptor *r_descriptor) {
	ERR_FAIL_COND_MSG(get_script_instance() == nullptr, "[FATAL] This is not supposed to happen.");

	system_descriptor = r_descriptor;

	Callable::CallError err;
	get_script_instance()->call("_prepare", nullptr, 0, err);

	system_descriptor = nullptr;
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
	bool has_for_each = false;
	for (List<MethodInfo>::Element *e = methods.front(); e; e = e->next()) {
		if (e->get().name == "_prepare") {
			has_prepare = true;
		}
		if (e->get().name == "_for_each") {
			has_for_each = true;
			// TODO consider add input check, so to notify the user if the system is not valid.
		}
	}
	if (has_prepare == false) {
		return TTR("This script is not overriding the function `_prepare()`.");
	}
	if (has_for_each == false) {
		return TTR("This script is not overriding the function `_for_each()`.");
	}

	List<PropertyInfo> properties;
	p_script->get_script_property_list(&properties);
	if (properties.size()) {
		return TTR("The System script can't have any property in it. It possible to only access `Component`s and `Databag`s.");
	}

	// This script is safe to use.
	return "";
}

void SystemBundle::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add", "system_name"), &SystemBundle::add);
	ClassDB::bind_method(D_METHOD("with_description", "desc"), &SystemBundle::with_description);
	ClassDB::bind_method(D_METHOD("before", "dependency"), &SystemBundle::before);
	ClassDB::bind_method(D_METHOD("after", "dependency"), &SystemBundle::after);
}

void SystemBundle::__fetch_descriptor(SystemBundleDescriptor *r_descriptor) {
	ERR_FAIL_COND_MSG(get_script_instance() == nullptr, "[FATAL] This is not supposed to happen.");
	CRASH_COND_MSG(r_descriptor == nullptr, "r_desc is a requited variable.");

	descriptor = r_descriptor;

	Callable::CallError err;
	get_script_instance()->call("_prepare", nullptr, 0, err);

	descriptor = nullptr;
}

const String &SystemBundle::get_script_path() const {
	return script_path;
}

void SystemBundle::add(const StringName &p_system_name) {
	ERR_FAIL_COND_MSG(descriptor == nullptr, "Never call `_prepare` directly. Use `__fetch_descriptor` instead.");
	descriptor->systems.push_back(p_system_name);
}

void SystemBundle::with_description(const String &p_desc) {
	ERR_FAIL_COND_MSG(descriptor == nullptr, "Never call `_prepare` directly. Use `__fetch_descriptor` instead.");
	descriptor->description = p_desc;
}

void SystemBundle::before(const StringName &p_dependency) {
	ERR_FAIL_COND_MSG(descriptor == nullptr, "Never call `_prepare` directly. Use `__fetch_descriptor` instead.");
	descriptor->dependencies.push_back({ true, p_dependency });
}

void SystemBundle::after(const StringName &p_dependency) {
	ERR_FAIL_COND_MSG(descriptor == nullptr, "Never call `_prepare` directly. Use `__fetch_descriptor` instead.");
	descriptor->dependencies.push_back({ false, p_dependency });
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

void Component::internal_set_component_script(Ref<Script> p_script) {
	component_script = p_script;
}

StringName Component::get_name() const {
	return name;
}

void Component::get_component_property_list(List<PropertyInfo> *r_info) {
	if (component_script.is_null()) {
		return;
	}

	component_script->get_script_property_list(r_info);
}

Variant Component::get_property_default_value(StringName p_property_name) {
	// TODO this function is EXTREMELY bad! create a `ScriptInstance` is an
	// TODO heavy task. Keep a script instance alive in editor seems unsafe because
	// TODO the script can change anytime and all the instances are immediately
	// TODO invalidated.
	// TODO Please optimize it.
	WARN_PRINT_ONCE("!IMPORTANT [TODO] please optimize the GDScript component get defaul val which is extremely slow!!!!!");

	ERR_FAIL_COND_V(component_script.is_null(), Variant());
	ScriptInstance *si = component_script->instance_create(this);
	ERR_FAIL_COND_V(si == nullptr, Variant());
	Variant ret;
	si->get(p_property_name, ret);
	// Make sure to clear the script, so it's correctly destroyed.
	set_script_instance(nullptr);
	set_script(Ref<Script>());
	return ret;
}

Vector<StringName> Component::get_spawners() {
	ERR_FAIL_COND_V(component_script.is_null(), Vector<StringName>());

	ScriptInstance *si = component_script->instance_create(this);
	ERR_FAIL_COND_V(si == nullptr, Vector<StringName>());

	Vector<StringName> spawners;
	if (si->has_method("get_spawners")) {
		spawners = si->call("get_spawners");
	}

	// Make sure to clear the script, so it's correctly destroyed.
	set_script_instance(nullptr);
	set_script(Ref<Script>());

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

	const LocalVector<PropertyInfo> *props = ECS::get_component_properties(component_id);
	for (uint32_t i = 0; i < props->size(); i += 1) {
		Variant v;
		ECS::unsafe_component_get_by_index(component_id, component, i, v);
		d[StringName((*props)[i].name)] = v;
	}

	return d;
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
		return ScriptEcs::get_singleton()->component_get_property_default_value(component_name, p_name, r_ret);
	} else {
		r_ret = v->duplicate(true);
		return true;
	}
}

Dictionary ScriptComponentDepot::get_properties_data() const {
	return data;
}

void SharedComponentDepot::init(const StringName &p_name) {
	ERR_FAIL_COND_MSG(component_name != StringName(), "The component is already initialized.");
	ERR_FAIL_COND_MSG(ScriptEcs::get_singleton()->component_is_shared(p_name) == false, "Thid component " + p_name + " is not a shared component.");
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
			data.instance();
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
			return ScriptEcs::get_singleton()->component_get_property_default_value(component_name, p_name, r_ret);
		} else {
			return true;
		}
	}
	return false;
}

Dictionary SharedComponentDepot::get_properties_data() const {
	Dictionary d;
	d[StringName("resource")] = data;
	return d;
}
