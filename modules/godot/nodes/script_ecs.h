#pragma once

#include "../../../ecs.h"
#include "core/io/resource.h" // TODO remove this or the one on the cpp?
#include "core/templates/local_vector.h"
#include "core/templates/oa_hash_map.h"
#include "ecs_utilities.h"

class EditorFileSystemDirectory;

/// This is an utility that is intended for in editor usage that allow:
/// - Save script components.
/// - Load script components.
/// - Get script components properties.
/// - Save script databags.
/// - Load script databags.
/// - Save script systems.
/// - Load script systems.
class ScriptEcs : public Object {
	GDCLASS(ScriptEcs, Object)

	uint64_t recent_modification_detected_time = 0;

	bool def_defined_static_components = false;
	/// Used to know if the ScriptedEcs components are registered.
	bool ecs_initialized = false;

	/// List of script components each spawner can spawn.
	/// @Key: Spawner name
	/// @Value: List of script components
	OAHashMap<StringName, Set<StringName>> spawners;

	LocalVector<StringName> component_names;
	LocalVector<Ref<Component>> components;

	LocalVector<StringName> system_names;
	LocalVector<Ref<System>> systems;

	LocalVector<StringName> system_bundle_names;
	LocalVector<Ref<SystemBundle>> system_bundles;

	static ScriptEcs *singleton;

public:
	ScriptEcs();
	~ScriptEcs();

	static ScriptEcs *get_singleton();

	// ------------------------------------------------------------------ Spawner
	/// Returns the components name this spawner can spawn.
	Vector<StringName> spawner_get_components(const StringName &spawner_name);

	// ---------------------------------------------------------------- Component
	/// Load or Reloads a component. Retuns the component id.

	/// Returns all the scripted components.
	const LocalVector<Ref<Component>> &get_components();

	/// Returns a script component.
	Ref<Component> get_script_component(const StringName &p_name);

	void component_get_properties(const StringName &p_component_name, List<PropertyInfo> *r_properties);
	bool component_get_property_default_value(const StringName &p_component_name, const StringName &p_property_name, Variant &r_ret);
	bool component_is_shared(const StringName &p_component_name);

	// ------------------------------------------------------------------ Databag

	// ----------------------------------------------------------- System Bundles
	Ref<SystemBundle> get_script_system_bundle(const StringName &p_name);

	// ------------------------------------------------------------------- System
	const LocalVector<StringName> &get_script_system_names();
	const LocalVector<Ref<System>> &get_script_systems();

	Ref<System> get_script_system(const StringName &p_name);

	// ------------------------------------------------------------------ Runtime

	void reload_scripts();
	uint64_t load_scripts(EditorFileSystemDirectory *p_dir);

	void define_editor_default_component_properties();

	void register_runtime_scripts();
	void register_dynamic_components();
	void register_dynamic_component(Component *p_component);
	void register_dynamic_systems();
	void register_dynamic_system_bundles();

private:
	void reload_script(const String &p_path, const String &p_name, const bool p_force_reload);
	Ref<SystemBundle> reload_system_bundle(Ref<Script> p_script, const String &p_path, const String &p_name);
	Ref<System> reload_system(Ref<Script> p_script, const String &p_path, const String &p_name);
	Ref<Component> reload_component(Ref<Script> p_script, const String &p_path, const String &p_name);

	void save_script(const String &p_setting_list_name, const String &p_script_path);
	void remove_script(const String &p_setting_list_name, const String &p_script_path);
};
