#include "ecs_utilities.h"

#include "../../../ecs.h"
#include "../../../systems/dynamic_system.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "core/object/script_language.h"
#include "editor/editor_node.h"

void System::_bind_methods() {
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

void System::__force_set_system_info(godex::DynamicSystemInfo *p_info, godex::system_id p_id) {
	id = p_id;
	info = p_info;
}

System::System() {
}

System::~System() {
	if (id != UINT32_MAX) {
		ECS::set_dynamic_system_target(id, nullptr);
	}
}

void System::set_space(Space p_space) {
	ERR_FAIL_COND_MSG(prepare_in_progress == false, "No info set. This function can be called only within the `_prepare`.");
	info->set_space(p_space);
}

void System::with_databag(uint32_t p_databag_id, Mutability p_mutability) {
	ERR_FAIL_COND_MSG(prepare_in_progress == false, "No info set. This function can be called only within the `_prepare`.");
	info->with_databag(p_databag_id, p_mutability == MUTABLE);
}

void System::with_storage(uint32_t p_component_id) {
	ERR_FAIL_COND_MSG(prepare_in_progress == false, "No info set. This function can be called only within the `_prepare`.");
	info->with_storage(p_component_id);
}

void System::with_component(uint32_t p_component_id, Mutability p_mutability) {
	ERR_FAIL_COND_MSG(prepare_in_progress == false, "No info set. This function can be called only within the `_prepare`.");
	info->with_component(p_component_id, p_mutability == MUTABLE);
}

void System::maybe_component(uint32_t p_component_id, Mutability p_mutability) {
	ERR_FAIL_COND_MSG(prepare_in_progress == false, "No info set. This function can be called only within the `_prepare`.");
	info->maybe_component(p_component_id, p_mutability == MUTABLE);
}

void System::changed_component(uint32_t p_component_id, Mutability p_mutability) {
	ERR_FAIL_COND_MSG(prepare_in_progress == false, "No info set. This function can be called only within the `_prepare`.");
	info->changed_component(p_component_id, p_mutability == MUTABLE);
}

void System::not_component(uint32_t p_component_id) {
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

String System::validate_script(Ref<Script> p_script) {
	ERR_FAIL_COND_V(p_script.is_null(), "Script is null.");
	ERR_FAIL_COND_V(p_script->is_valid() == false, "Script has some errors.");
	ERR_FAIL_COND_V("System" != p_script->get_instance_base_type(), "This script is not extending `System`.");

	List<PropertyInfo> properties;
	p_script->get_script_property_list(&properties);
	if (properties.size()) {
		return TTR("The System script can't have any property in it. It possible to only access `Component`s and `Databag`s.");
	}

	// This script is safe to use.
	return "";
}

void Component::_bind_methods() {}

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
	ERR_FAIL_COND_V(p_script.is_null(), "Script is null.");
	ERR_FAIL_COND_V(p_script->is_valid() == false, "Script has some errors.");
	ERR_FAIL_COND_V("Component" != p_script->get_instance_base_type(), "This script is not extending `Component`.");

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

bool EditorEcs::component_loaded = false;
bool EditorEcs::systems_loaded = false;
bool EditorEcs::ecs_initialized = false;

OAHashMap<StringName, Set<StringName>> EditorEcs::spawners;
LocalVector<StringName> EditorEcs::component_names;
LocalVector<Ref<Component>> EditorEcs::components;
LocalVector<StringName> EditorEcs::system_names;
LocalVector<Ref<System>> EditorEcs::systems;

void EditorEcs::__static_destructor() {
	component_names.reset();
	components.reset();
	system_names.reset();
	systems.reset();
}

Vector<StringName> EditorEcs::spawner_get_components(const StringName &spawner_name) {
	Vector<StringName> ret;

	// If in editor, extracts the spawnable components.
	if (Engine::get_singleton()->is_editor_hint()) {
		Set<StringName> *spawnable_components = spawners.lookup_ptr(spawner_name);
		if (spawnable_components != nullptr) {
			for (Set<StringName>::Element *e = spawnable_components->front(); e; e = e->next()) {
				ret.push_back(e->get());
			}
		}
	}

	// Now extract the C++ spawnable components.
	const godex::spawner_id spawner = ECS::get_spawner_id(spawner_name);
	if (spawner != godex::SPAWNER_NONE) {
		const LocalVector<godex::component_id> &spawnable_components = ECS::get_spawnable_components(spawner);
		for (uint32_t i = 0; i < spawnable_components.size(); i += 1) {
			ret.push_back(ECS::get_component_name(spawnable_components[i]));
		}
	}

	return ret;
}

void EditorEcs::load_components() {
	if (component_loaded) {
		return;
	}
	component_loaded = true;

	if (ProjectSettings::get_singleton()->has_setting("ECS/Component/scripts") == false) {
		return;
	}

	const Array scripts = ProjectSettings::get_singleton()->get_setting("ECS/Component/scripts");
	for (int i = 0; i < scripts.size(); i += 1) {
		reload_component(scripts[i]);
	}
}

StringName EditorEcs::reload_component(const String &p_path) {
	const StringName name = p_path.get_file();
	if (is_script_component(name) == false) {
		// Component doesn't exists.

		Ref<Script> script = ResourceLoader::load(p_path);

		ERR_FAIL_COND_V_MSG(script.is_null(), StringName(), "The script [" + p_path + "] can't be loaded.");
		ERR_FAIL_COND_V_MSG(script->is_valid() == false, StringName(), "The script [" + p_path + "] is not a valid script.");
		ERR_FAIL_COND_V_MSG("Component" != script->get_instance_base_type(), StringName(), "This script [" + p_path + "] is not extending `Component`.");
		const String res = Component::validate_script(script);
		ERR_FAIL_COND_V_MSG(res != "", StringName(), "This script [" + p_path + "] is not valid: " + res);

		Ref<Component> component;
		component.instance();
		component->internal_set_name(name);
		component->internal_set_component_script(script);

		component_names.push_back(name);
		components.push_back(component);

		if (Engine::get_singleton()->is_editor_hint()) {
			// In editor, fetch the spawners.

			Vector<StringName> comp_spawners = component->get_spawners();
			for (int i = 0; i < comp_spawners.size(); i += 1) {
				Set<StringName> *spawner_components = spawners.lookup_ptr(comp_spawners[i]);
				if (spawner_components == nullptr) {
					spawners.insert(comp_spawners[i], Set<StringName>());
					spawner_components = spawners.lookup_ptr(comp_spawners[i]);
				}
				spawner_components->insert(name);
			}
		}
	}
	return name;
}

const LocalVector<Ref<Component>> &EditorEcs::get_components() {
	load_components();

	return components;
}

bool EditorEcs::is_script_component(const StringName &p_name) {
	const int64_t index = component_names.find(p_name);
	return index >= 0;
}

Ref<Component> EditorEcs::get_script_component(const StringName &p_name) {
	load_components();

	const int64_t index = component_names.find(p_name);
	return index < 0 ? Ref<Component>() : components[index];
}

void EditorEcs::component_get_properties(const StringName &p_component_name, List<PropertyInfo> *r_properties) {
	if (Engine::get_singleton()->is_editor_hint() &&
			is_script_component(p_component_name)) {
		Ref<Component> component = EditorEcs::get_script_component(p_component_name);
		if (component.is_valid()) {
			component->get_component_property_list(r_properties);
		}
	} else {
		const godex::component_id component = ECS::get_component_id(p_component_name);
		ERR_FAIL_COND_MSG(component == godex::COMPONENT_NONE, "The component " + p_component_name + " doesn't exists.");

		const LocalVector<PropertyInfo> *props = ECS::get_component_properties(component);
		for (uint32_t i = 0; i < props->size(); i += 1) {
			r_properties->push_back((*props)[i]);
		}
	}
}

bool EditorEcs::component_get_property_default_value(const StringName &p_component_name, const StringName &p_property_name, Variant &r_ret) {
	if (Engine::get_singleton()->is_editor_hint() &&
			is_script_component(p_component_name)) {
		if (component_is_shared(p_component_name)) {
			// TODO by default the shared component is null?
			r_ret = Variant();
			return true;
		} else {
			// This is a Script Component and we are on editor.
			Ref<Component> c = get_script_component(p_component_name);
			r_ret = c->get_property_default_value(p_property_name);
			return true;
		}
	} else {
		// We are not on editor or this is a native component.
		const godex::component_id id = ECS::get_component_id(p_component_name);
		ERR_FAIL_COND_V_MSG(id == UINT32_MAX, false, "The component " + p_component_name + " doesn't exists.");
		r_ret = ECS::get_component_property_default(id, p_property_name);
		return true;
	}
}

bool EditorEcs::component_is_shared(const StringName &p_component_name) {
	if (is_script_component(p_component_name)) {
		// TODO Not yet supported in GDScript.
		return false;
	} else {
		return ECS::is_component_sharable(ECS::get_component_id(p_component_name));
	}
}

String EditorEcs::component_save_script(const String &p_script_path, Ref<Script> p_script) {
	// The script is valid, store it.
	ERR_FAIL_COND_V_MSG(Engine::get_singleton()->is_editor_hint() == false, "Not in editor.", "Not in editor.");

	String err = Component::validate_script(p_script);
	if (err != "") {
		// Component not valid.
		return String(TTR("The script [")) + p_script_path + String(TTR("] validation failed: ")) + err;
	}

	// Save script path.
	if (save_script("ECS/Component/scripts", p_script_path) == false) {
		return String(TTR("The")) + " Component [" + p_script_path + "] " + TTR("is already registered.");
	}

	// Load this component script so we can operate on it.
	EditorEcs::reload_component(p_script_path);

	// Success
	return "";
}

void EditorEcs::load_systems() {
	if (systems_loaded) {
		return;
	}
	systems_loaded = true;

	if (ProjectSettings::get_singleton()->has_setting("ECS/System/scripts") == false) {
		return;
	}

	const Array scripts = ProjectSettings::get_singleton()->get_setting("ECS/System/scripts");
	for (int i = 0; i < scripts.size(); i += 1) {
		reload_system(scripts[i]);
	}
}

StringName EditorEcs::reload_system(const String &p_path) {
	const StringName name = p_path.get_file();
	if (is_script_system(name) == false) {
		// System doesn't exists.

		Ref<Script> script = ResourceLoader::load(p_path);

		ERR_FAIL_COND_V_MSG(script.is_null(), StringName(), "The script [" + p_path + "] can't be loaded.");
		ERR_FAIL_COND_V_MSG(script->is_valid() == false, StringName(), "The script [" + p_path + "] is not a valid script.");
		ERR_FAIL_COND_V_MSG("System" != script->get_instance_base_type(), StringName(), "This script [" + p_path + "] is not extending `Component`.");
		const String res = System::validate_script(script);
		ERR_FAIL_COND_V_MSG(res != "", StringName();, "This script [" + p_path + "] is not valid: " + res);

		Ref<System> system;
		system.instance();

		system_names.push_back(name);
		systems.push_back(system);

		system->set_script(script);
	}
	return name;
}

bool EditorEcs::is_script_system(const StringName &p_name) {
	const int64_t index = system_names.find(p_name);
	return index >= 0;
}

String EditorEcs::system_save_script(const String &p_script_path, Ref<Script> p_script) {
	// The script is valid, store it.
	ERR_FAIL_COND_V_MSG(Engine::get_singleton()->is_editor_hint() == false, "Not in editor.", "Not in editor.");

	String err = System::validate_script(p_script);
	if (err != "") {
		// System not valid.
		return String(TTR("The script [")) + p_script_path + String(TTR("] validation failed: ")) + err;
	}

	// Save script path.
	if (save_script("ECS/System/scripts", p_script_path) == false) {
		return String(TTR("The")) + " System [" + p_script_path + "] " + TTR("is already registered.");
	}

	// Make this script available so we can operate on it.
	reload_system(p_script_path);

	// Success
	return "";
}

void EditorEcs::register_runtime_scripts() {
	if (Engine::get_singleton()->is_editor_hint()) {
		// Only when the editor is off the Scripted components are registered.
		return;
	}

	if (ecs_initialized) {
		return;
	}

	ecs_initialized = true;

	register_dynamic_components();
	// TODO register databags
	register_dynamic_systems();
}

void EditorEcs::register_dynamic_components() {
	load_components();

	for (uint32_t i = 0; i < components.size(); i += 1) {
		components[i]->internal_set_name(component_names[i]);
		register_dynamic_component(components[i].ptr());
	}
}

void EditorEcs::register_dynamic_component(Component *p_component) {
	List<PropertyInfo> raw_properties;
	p_component->get_component_property_list(&raw_properties);

	LocalVector<ScriptProperty> properties;
	properties.reserve(raw_properties.size());
	for (List<PropertyInfo>::Element *e = raw_properties.front(); e; e = e->next()) {
		properties.push_back({ e->get(),
				// TODO use a way to get all the values at once.
				p_component->get_property_default_value(e->get().name) });
	}

	ECS::register_script_component(
			p_component->get_name(),
			properties,
			// TODO make the storage customizable.
			StorageType::DENSE_VECTOR,
			p_component->get_spawners());
}

void EditorEcs::register_dynamic_systems() {
	load_systems();

	for (uint32_t i = 0; i < systems.size(); i += 1) {
		systems[i]->id = ECS::register_dynamic_system(system_names[i]);
		systems[i]->prepare(
				ECS::get_dynamic_system_info(systems[i]->id),
				systems[i]->id);
	}
}

bool EditorEcs::save_script(const String &p_setting_list_name, const String &p_script_path) {
	ERR_FAIL_COND_V_MSG(EditorNode::get_singleton() == nullptr, false, "The editor is not defined.");

	Array scripts;
	if (ProjectSettings::get_singleton()->has_setting(p_setting_list_name)) {
		scripts = ProjectSettings::get_singleton()->get_setting(p_setting_list_name);
	}

	if (scripts.find(p_script_path) >= 0) {
		// Script already stored.
		return false;
	} else {
		Array prev_scripts = scripts.duplicate(true);
		scripts.push_back(p_script_path);

		EditorNode::get_undo_redo()->create_action(TTR("Save script " + p_setting_list_name));
		EditorNode::get_undo_redo()->add_do_method(ProjectSettings::get_singleton(), "set_setting", p_setting_list_name, scripts);
		EditorNode::get_undo_redo()->add_undo_method(ProjectSettings::get_singleton(), "set_setting", p_setting_list_name, prev_scripts);
		EditorNode::get_undo_redo()->commit_action();

		return true;
	}
}
