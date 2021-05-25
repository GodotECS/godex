#include "script_ecs.h"

#include "../../../ecs.h"
#include "../../../systems/dynamic_system.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "core/object/script_language.h"
#include "editor/editor_file_system.h"
#include "editor/editor_node.h"
#include "entity.h"
#include "shared_component_resource.h"

ScriptEcs *ScriptEcs::singleton = nullptr;

ScriptEcs::ScriptEcs() {
	singleton = this;
}

ScriptEcs *ScriptEcs::get_singleton() {
	return singleton;
}

ScriptEcs::~ScriptEcs() {
	singleton = nullptr;
}

Vector<StringName> ScriptEcs::spawner_get_components(const StringName &spawner_name) {
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

void ScriptEcs::load_components() {
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

StringName ScriptEcs::reload_component(const String &p_path) {
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

const LocalVector<Ref<Component>> &ScriptEcs::get_components() {
	load_components();

	return components;
}

bool ScriptEcs::is_script_component(const StringName &p_name) {
	const int64_t index = component_names.find(p_name);
	return index >= 0;
}

Ref<Component> ScriptEcs::get_script_component(const StringName &p_name) {
	load_components();

	const int64_t index = component_names.find(p_name);
	return index < 0 ? Ref<Component>() : components[index];
}

void ScriptEcs::component_get_properties(const StringName &p_component_name, List<PropertyInfo> *r_properties) {
	if (Engine::get_singleton()->is_editor_hint() &&
			is_script_component(p_component_name)) {
		Ref<Component> component = ScriptEcs::get_script_component(p_component_name);
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

bool ScriptEcs::component_get_property_default_value(const StringName &p_component_name, const StringName &p_property_name, Variant &r_ret) {
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

bool ScriptEcs::component_is_shared(const StringName &p_component_name) {
	if (is_script_component(p_component_name)) {
		// TODO Not yet supported in GDScript.
		return false;
	} else {
		return ECS::is_component_sharable(ECS::get_component_id(p_component_name));
	}
}

String ScriptEcs::component_save_script(const String &p_script_path, Ref<Script> p_script) {
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
	ScriptEcs::reload_component(p_script_path);

	// Success
	return "";
}

void ScriptEcs::add_system_to_bundle(const StringName &p_system_name, const StringName &p_system_bundle_name) {
	int64_t index = system_bundle_names.find(p_system_bundle_name);
	if (index == -1) {
		index = system_bundle_names.size();
		system_bundle_names.push_back(p_system_bundle_name);
		system_bundles.push_back(SystemBundle());
	}

	system_bundles[index].systems.push_back(p_system_name);
}

void ScriptEcs::load_systems() {
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

StringName ScriptEcs::reload_system(const String &p_path) {
	const StringName name = p_path.get_file();
	if (is_script_system(name) == false) {
		// System doesn't exists.

		Ref<Script> script = ResourceLoader::load(p_path);

		ERR_FAIL_COND_V_MSG(script.is_null(), StringName(), "The script [" + p_path + "] can't be loaded.");
		ERR_FAIL_COND_V_MSG(script->is_valid() == false, StringName(), "The script [" + p_path + "] is not a valid script.");
		ERR_FAIL_COND_V_MSG("System" != script->get_instance_base_type(), StringName(), "This script [" + p_path + "] is not extending `Component`.");
		const String res = System::validate_script(script);
		ERR_FAIL_COND_V_MSG(res != "", StringName(), "This script [" + p_path + "] is not valid: " + res);

		Ref<System> system;
		system.instance();

		system_names.push_back(name);
		systems.push_back(system);

		system->set_script(script);
	}
	return name;
}

bool ScriptEcs::is_script_system(const StringName &p_name) {
	const int64_t index = system_names.find(p_name);
	return index >= 0;
}

String ScriptEcs::system_save_script(const String &p_script_path, Ref<Script> p_script) {
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

void ScriptEcs::reload_scripts() {
	// Scan the script classes.
	if (EditorFileSystem::get_singleton()->get_filesystem()) {
		const uint64_t modificatio_time =
				load_scripts(EditorFileSystem::get_singleton()->get_filesystem());
		recent_modification_detected_time =
				MAX(recent_modification_detected_time, modificatio_time);
	}
}

uint64_t ScriptEcs::load_scripts(EditorFileSystemDirectory *p_dir) {
	uint64_t recent_modification = 0;
	for (int i = 0; i < p_dir->get_file_count(); i += 1) {
		if (p_dir->get_file_import_is_valid(i) == false) {
			continue;
		} else if (p_dir->get_file_modified_time(i) <= recent_modification_detected_time) {
			continue;
		} else if (p_dir->get_file_script_class_extends(i) == "System") {
			print_line("This file is a System: " + p_dir->get_file_path(i));
		} else if (p_dir->get_file_script_class_extends(i) == "Component") {
			print_line("This file is a Component : " + p_dir->get_file_path(i));
		} else {
			continue;
		}

		recent_modification = MAX(recent_modification, p_dir->get_file_modified_time(i));
	}
	for (int i = 0; i < p_dir->get_subdir_count(); i += 1) {
		const uint64_t dir_modification_time = load_scripts(p_dir->get_subdir(i));
		recent_modification = MAX(recent_modification, dir_modification_time);
	}
	return recent_modification;
}

void ScriptEcs::define_editor_default_component_properties() {
	const StringName entity_3d_name = Entity3D::get_class_static();
	const StringName entity_2d_name = Entity2D::get_class_static();

	if (def_defined_static_components == false) {
		def_defined_static_components = true;
		for (godex::component_id id = 0; id < ECS::get_components_count(); id += 1) {
			const LocalVector<PropertyInfo> *props = ECS::get_component_properties(id);
			for (uint32_t p = 0; p < props->size(); p += 1) {
				Variant def = ECS::get_component_property_default(id, (*props)[p].name);
				ClassDB::set_property_default_value(entity_3d_name, StringName(String(ECS::get_component_name(id)) + "/" + (*props)[p].name), def);
				ClassDB::set_property_default_value(entity_2d_name, StringName(String(ECS::get_component_name(id)) + "/" + (*props)[p].name), def);
			}
		}
	}

	// Register the scripted component defaults.
	for (uint32_t i = 0; i < components.size(); i += 1) {
		List<PropertyInfo> props;
		components[i]->get_property_list(&props);
		for (List<PropertyInfo>::Element *e = props.front(); e; e = e->next()) {
			Variant def = components[i]->get_property_default_value(e->get().name);
			ClassDB::set_property_default_value(entity_3d_name, StringName(String(component_names[i]) + "/" + e->get().name), def);
			ClassDB::set_property_default_value(entity_2d_name, StringName(String(component_names[i]) + "/" + e->get().name), def);
		}
	}
}

void ScriptEcs::register_runtime_scripts() {
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

void ScriptEcs::register_dynamic_components() {
	load_components();

	for (uint32_t i = 0; i < components.size(); i += 1) {
		components[i]->internal_set_name(component_names[i]);
		register_dynamic_component(components[i].ptr());
	}
}

void ScriptEcs::register_dynamic_component(Component *p_component) {
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

void ScriptEcs::register_dynamic_systems() {
	load_systems();

	for (uint32_t i = 0; i < systems.size(); i += 1) {
		systems[i]->id = ECS::register_dynamic_system(system_names[i]).get_id();
		systems[i]->prepare(
				ECS::get_dynamic_system_info(systems[i]->id),
				systems[i]->id);
	}
}

void ScriptEcs::fetch_systems_execution_info() {
	load_systems();

	system_bundle_names.clear();
	system_bundles.clear();

	for (uint32_t i = 0; i < systems.size(); i += 1) {
		systems[i]->fetch_execution_data();
	}
}

bool ScriptEcs::save_script(const String &p_setting_list_name, const String &p_script_path) {
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
		EditorNode::get_undo_redo()->add_do_method(ProjectSettings::get_singleton(), "save");
		EditorNode::get_undo_redo()->add_undo_method(ProjectSettings::get_singleton(), "set_setting", p_setting_list_name, prev_scripts);
		EditorNode::get_undo_redo()->add_undo_method(ProjectSettings::get_singleton(), "save");
		EditorNode::get_undo_redo()->commit_action();

		return true;
	}
}
