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
	OAHashMap<StringName, RBSet<StringName>> spawners;

	LocalVector<StringName> component_names;
	LocalVector<Ref<Component>> components;

	LocalVector<StringName> system_names;
	LocalVector<Ref<System>> systems;

	LocalVector<StringName> system_bundle_names;
	LocalVector<Ref<SystemBundle>> system_bundles;

	// This vector contains the `System` and `SystemBundle` which preparation is
	// still in pending. When a new script is fetched its constant id is immediately
	// registered, so it's possible to fetch it using `ECS.MySystemName`.
	// The preparation is delayed, so it can be executed when all the systems are
	// known, and the various dependency can be safely resolved using the
	// syntax: `ECS.MySystemName`.
	LocalVector<Ref<Resource>> scripts_with_pending_prepare;

	static ScriptEcs *singleton;

	static void _bind_methods();

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

	// ------------------------------------------------------------------ Databag

	// ----------------------------------------------------------- System Bundles
	Ref<SystemBundle> get_script_system_bundle(const StringName &p_name) const;

	// ------------------------------------------------------------------- System
	const LocalVector<StringName> &get_script_system_names();
	const LocalVector<Ref<System>> &get_script_systems();

	Ref<System> get_script_system(const StringName &p_name);

	// ------------------------------------------------------------------ Runtime

	void reload_scripts();
	uint64_t load_scripts(EditorFileSystemDirectory *p_dir);

	void define_editor_default_component_properties();
	void reset_editor_default_component_properties();

	void register_runtime_scripts();

	void __empty_scripts();
	bool __reload_script(const String &p_path, const String &p_name, const bool p_force_reload);
	bool __reload_script(Ref<Script> p_script, const String &p_path, const String &p_name);
	Ref<SystemBundle> __reload_system_bundle(Ref<Script> p_script, const String &p_path, const String &p_name);
	Ref<System> __reload_system(Ref<Script> p_script, const String &p_path, const String &p_name);
	Ref<Component> __reload_component(Ref<Script> p_script, const String &p_path, const String &p_name);

	void flush_scripts_preparation();

private:
	void save_script(const String &p_setting_list_name, const String &p_script_path);
	void remove_script(const String &p_setting_list_name, const String &p_script_path);
};
