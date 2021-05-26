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

	component_names.reset();
	components.reset();

	system_names.reset();
	systems.reset();
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

const LocalVector<Ref<Component>> &ScriptEcs::get_components() {
	return components;
}

Ref<Component> ScriptEcs::get_script_component(const StringName &p_name) {
	const int64_t index = component_names.find(p_name);
	return index < 0 ? Ref<Component>() : components[index];
}

void ScriptEcs::component_get_properties(const StringName &p_component_name, List<PropertyInfo> *r_properties) {
	Ref<Component> script_component = ScriptEcs::get_script_component(p_component_name);
	if (Engine::get_singleton()->is_editor_hint() && script_component.is_valid()) {
		script_component->get_component_property_list(r_properties);
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
	if (component_is_shared(p_component_name)) {
		// TODO by default the shared component is null?
		r_ret = Variant();
		return true;
	}

	Ref<Component> script_component = get_script_component(p_component_name);
	if (Engine::get_singleton()->is_editor_hint() && script_component.is_valid()) {
		// This is a Script Component and we are on editor.
		r_ret = script_component->get_property_default_value(p_property_name);
		return true;
	} else {
		// We are not on editor or this is a native component.
		const godex::component_id id = ECS::get_component_id(p_component_name);
		ERR_FAIL_COND_V_MSG(id == UINT32_MAX, false, "The component " + p_component_name + " doesn't exists.");
		r_ret = ECS::get_component_property_default(id, p_property_name);
		return true;
	}
}

bool ScriptEcs::component_is_shared(const StringName &p_component_name) {
	if (get_script_component(p_component_name).is_valid()) {
		// TODO Not yet supported in GDScript.
		return false;
	} else {
		return ECS::is_component_sharable(ECS::get_component_id(p_component_name));
	}
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

const LocalVector<StringName> &ScriptEcs::get_script_system_names() {
	return system_names;
}

const LocalVector<Ref<System>> &ScriptEcs::get_script_systems() {
	return systems;
}

Ref<System> ScriptEcs::get_script_system(const StringName &p_name) {
	const int64_t index = system_names.find(p_name);
	return index >= 0 ? systems[index] : Ref<System>();
}

void ScriptEcs::reload_scripts() {
	// Mark the systems and components as not verified. This allow to detect the
	// deleted scripts.
	for (uint32_t i = 0; i < systems.size(); i += 1) {
		systems[i]->verified = false;
	}
	for (uint32_t i = 0; i < components.size(); i += 1) {
		components[i]->verified = false;
	}

	// Scan the script classes.
	if (EditorFileSystem::get_singleton()->get_filesystem()) {
		const uint64_t modificatio_time =
				load_scripts(EditorFileSystem::get_singleton()->get_filesystem());
		recent_modification_detected_time =
				MAX(recent_modification_detected_time, modificatio_time);
	}

	for (int i = int(systems.size()) - 1; i >= 0; i -= 1) {
		if (systems[i]->verified == false) {
			// Remove the path from the stored systems.
			remove_script("ECS/Autoload/scripts", systems[i]->get_script_path());
			// The script for this system doesn't exist anymore.
			// Remove it.
			system_names.remove_unordered(i);
			systems.remove_unordered(i);
		}
	}
	for (int i = int(components.size()) - 1; i >= 0; i -= 1) {
		if (components[i]->verified == false) {
			// Remove the path from the stored systems.
			remove_script("ECS/Autoload/scripts", components[i]->get_script_path());
			// The script for this system doesn't exist anymore.
			// Remove it.
			component_names.remove_unordered(i);
			components.remove_unordered(i);
		}
	}

	define_editor_default_component_properties();
}

uint64_t ScriptEcs::load_scripts(EditorFileSystemDirectory *p_dir) {
	uint64_t recent_modification = 0;
	for (int i = 0; i < p_dir->get_file_count(); i += 1) {
		const String file_type = p_dir->get_file_type(i);

		if (p_dir->get_file_import_is_valid(i) &&
				(file_type == "GDScript" ||
						file_type == "VisualScript" ||
						file_type == "NativeScript" ||
						file_type == "C#" ||
						file_type == "Rust")) { //TODO add more?

			const bool changed =
					p_dir->get_file_modified_time(i) > recent_modification_detected_time;
			reload_script(p_dir->get_file_path(i), p_dir->get_file(i), changed);
			recent_modification = MAX(recent_modification, p_dir->get_file_modified_time(i));
		}
	}

	// Load the script on the sub directories.
	for (int i = 0; i < p_dir->get_subdir_count(); i += 1) {
		const uint64_t dir_modification_time = load_scripts(p_dir->get_subdir(i));
		recent_modification = MAX(recent_modification, dir_modification_time);
	}
	return recent_modification;
}

void ScriptEcs::define_editor_default_component_properties() {
	const StringName entity_3d_name = Entity3D::get_class_static();
	const StringName entity_2d_name = Entity2D::get_class_static();

	for (godex::component_id id = 0; id < ECS::get_components_count(); id += 1) {
		const LocalVector<PropertyInfo> *props = ECS::get_component_properties(id);
		for (uint32_t p = 0; p < props->size(); p += 1) {
			Variant def = ECS::get_component_property_default(id, (*props)[p].name);
			ClassDB::set_property_default_value(entity_3d_name, StringName(String(ECS::get_component_name(id)) + "/" + (*props)[p].name), def);
			ClassDB::set_property_default_value(entity_2d_name, StringName(String(ECS::get_component_name(id)) + "/" + (*props)[p].name), def);
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

	if (ProjectSettings::get_singleton()->has_setting("ECS/Autoload/scripts") == false) {
		return;
	}

	const Array scripts = ProjectSettings::get_singleton()->get_setting("ECS/Autoload/scripts");
	for (int i = 0; i < scripts.size(); i += 1) {
		reload_script(scripts[i], String(scripts[i]).get_file(), true);
	}

	register_dynamic_components();
	// TODO register databags
	register_dynamic_systems();
}

void ScriptEcs::register_dynamic_components() {
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
	for (uint32_t i = 0; i < systems.size(); i += 1) {
		systems[i]->id = ECS::register_dynamic_system(system_names[i]).get_id();
		systems[i]->prepare(
				ECS::get_dynamic_system_info(systems[i]->id),
				systems[i]->id);
	}
}

void ScriptEcs::reload_script(const String &p_path, const String &p_name, const bool p_force_reload) {
	if (p_force_reload) {
		bool is_valid = false;

		Ref<Script> script = ResourceLoader::load(p_path);
		ERR_FAIL_COND(script.is_null());

		const StringName base_type = script->get_instance_base_type();
		if (base_type == "System") {
			Ref<System> system = reload_system(script, p_path, p_name);
			if (system.is_valid()) {
				system->verified = true;
				is_valid = true;
			}
		} else if (base_type == "Component") {
			Ref<Component> component = reload_component(script, p_path, p_name);
			if (component.is_valid()) {
				component->verified = true;
				is_valid = true;

				// Fetch the spawners.
				Vector<StringName> comp_spawners = component->get_spawners();
				for (int y = 0; y < comp_spawners.size(); y += 1) {
					Set<StringName> *spawner_components = spawners.lookup_ptr(comp_spawners[y]);
					if (spawner_components == nullptr) {
						spawners.insert(comp_spawners[y], Set<StringName>());
						spawner_components = spawners.lookup_ptr(comp_spawners[y]);
					}
					spawner_components->insert(p_name);
				}
			} else {
				if (p_force_reload) {
					// Make sure this component is not part of any spawner.
					for (OAHashMap<StringName, Set<StringName>>::Iterator it = spawners.iter(); it.valid; it = spawners.next_iter(it)) {
						it.value->erase(p_name);
					}
				}
			}
		} else {
			// Not an ecs script, nothing to do.
			return;
		}

		if (is_valid) {
			// Make sure the path is stored.
			save_script("ECS/Autoload/scripts", p_path);
		} else {
			// Make sure the path removed.
			remove_script("ECS/Autoload/scripts", p_path);
		}
	} else {
		Ref<System> system = get_script_system(p_name);
		if (system.is_valid()) {
			system->verified = true;
		} else {
			Ref<Component> component = get_script_component(p_name);
			if (component.is_valid()) {
				component->verified = true;
			}
		}
	}
}

Ref<System> ScriptEcs::reload_system(Ref<Script> p_script, const String &p_path, const String &p_name) {
	const StringName name = p_name;
	const int64_t index = system_names.find(name);
	Ref<System> system = index >= 0 ? systems[index] : Ref<System>();
	if (system.is_null()) {
		// System doesn't exists, create it.
		ERR_FAIL_COND_V_MSG(p_script.is_null(), Ref<System>(), "The script [" + p_path + "] can't be loaded.");
		ERR_FAIL_COND_V_MSG(p_script->is_valid() == false, Ref<System>(), "The script [" + p_path + "] is not a valid script.");
		ERR_FAIL_COND_V_MSG("System" != p_script->get_instance_base_type(), Ref<System>(), "This script [" + p_path + "] is not extending `Component`.");
		const String res = System::validate_script(p_script);
		ERR_FAIL_COND_V_MSG(res != "", Ref<System>(), "This script [" + p_path + "] is not a valid System: " + res);

		system.instance();
		system->set_script(p_script);
		system->script_path = p_path;

		system_names.push_back(name);
		systems.push_back(system);
	} else {
		// The system exists, validate it.
		const String res = System::validate_script(system->get_script());
		if (res != "") {
			// This script is no more valid, unlaod it.
			// Allignement between the vector is kept, since both use the same
			// removal logic.
			system_names.remove_unordered(index);
			systems.remove_unordered(index);

			ERR_FAIL_V_MSG(Ref<System>(), "This script [" + p_path + "] is not a valid System: " + res);
		}
	}

	return system;
}

Ref<Component> ScriptEcs::reload_component(Ref<Script> p_script, const String &p_path, const String &p_name) {
	const StringName name = p_name;
	const int64_t index = component_names.find(name);
	Ref<Component> component = index >= 0 ? components[index] : Ref<Component>();
	if (component.is_null()) {
		// Component doesn't exists yet.

		Ref<Script> script = ResourceLoader::load(p_path);

		ERR_FAIL_COND_V_MSG(script.is_null(), Ref<Component>(), "The script [" + p_path + "] can't be loaded.");
		ERR_FAIL_COND_V_MSG(script->is_valid() == false, Ref<Component>(), "The script [" + p_path + "] is not a valid script.");
		ERR_FAIL_COND_V_MSG("Component" != script->get_instance_base_type(), Ref<Component>(), "This script [" + p_path + "] is not extending `Component`.");
		const String res = Component::validate_script(script);
		ERR_FAIL_COND_V_MSG(res != "", Ref<Component>(), "This script [" + p_path + "] is not valid: " + res);

		component.instance();
		component->internal_set_name(name);
		component->internal_set_component_script(script);
		component->script_path = p_path;

		component_names.push_back(name);
		components.push_back(component);
	} else {
		const String res = Component::validate_script(component->get_script());
		if (res != "") {
			// This script is no more valid, unlaod it.
			// Allignement between the vector is kept, since both use the same
			// removal logic.
			component_names.remove_unordered(index);
			components.remove_unordered(index);

			ERR_FAIL_V_MSG(Ref<Component>(), "This script [" + p_path + "] is not a valid Component: " + res);
		}
	}

	return component;
}

void ScriptEcs::save_script(const String &p_setting_list_name, const String &p_script_path) {
	ERR_FAIL_COND_MSG(EditorNode::get_singleton() == nullptr, "The editor is not defined.");

	Array scripts;
	if (ProjectSettings::get_singleton()->has_setting(p_setting_list_name)) {
		scripts = ProjectSettings::get_singleton()->get_setting(p_setting_list_name);
	}

	if (scripts.find(p_script_path) < 0) {
		scripts.push_back(p_script_path);
		ProjectSettings::get_singleton()->set_setting(p_setting_list_name, scripts);
		ProjectSettings::get_singleton()->save();
	}
}

void ScriptEcs::remove_script(const String &p_setting_list_name, const String &p_script_path) {
	ERR_FAIL_COND_MSG(EditorNode::get_singleton() == nullptr, "The editor is not defined.");

	if (ProjectSettings::get_singleton()->has_setting(p_setting_list_name) == false) {
		// Nothing to do.
		return;
	}

	Array scripts = ProjectSettings::get_singleton()->get_setting(p_setting_list_name);
	const int index = scripts.find(p_script_path);

	if (index >= 0) {
		scripts.remove(index);
		ProjectSettings::get_singleton()->set_setting(p_setting_list_name, scripts);
		ProjectSettings::get_singleton()->save();
	}
}
