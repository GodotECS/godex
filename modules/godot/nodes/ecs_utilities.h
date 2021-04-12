#pragma once

#include "../../../ecs.h"
#include "core/io/resource.h" // TODO remove this or the one on the cpp?
#include "core/templates/local_vector.h"
#include "core/templates/oa_hash_map.h"

class Script;

namespace godex {
class DynamicSystemInfo;
}

class System : public Resource {
	GDCLASS(System, Resource);

	friend class EditorEcs;

	godex::DynamicSystemInfo *info = nullptr;
	godex::system_id id = UINT32_MAX;
	bool prepare_in_progress = false;

	static void _bind_methods();
	void prepare(godex::DynamicSystemInfo *p_info, godex::system_id p_id);

public:
	enum Mutability {
		IMMUTABLE,
		MUTABLE
	};

	void __force_set_system_info(godex::DynamicSystemInfo *p_info, godex::system_id p_id);

public:
	System();
	~System();

	void set_space(Space p_space);
	void with_databag(uint32_t p_databag_id, Mutability p_mutability);
	void with_storage(uint32_t p_component_id);
	void with_component(uint32_t p_component_id, Mutability p_mutability);
	void maybe_component(uint32_t p_component_id, Mutability p_mutability);
	void changed_component(uint32_t p_component_id, Mutability p_mutability);
	void not_component(uint32_t p_component_id);

	godex::system_id get_system_id() const;

	uint32_t get_current_entity_id() const;

	static String validate_script(Ref<Script> p_script);
};

VARIANT_ENUM_CAST(System::Mutability);

class Component : public Resource {
	GDCLASS(Component, Resource);

	friend class EditorEcs;

	StringName name;
	Ref<Script> component_script;

	static void _bind_methods();

public:
	Component();
	~Component();

	void internal_set_name(StringName p_name);
	void internal_set_component_script(Ref<Script> p_script);

	StringName get_name() const;

	void get_component_property_list(List<PropertyInfo> *p_info);
	Variant get_property_default_value(StringName p_property_name);

	Vector<StringName> get_spawners();

	/// Validate the script and returns a void string if the validation success
	/// or the error message.
	static String validate_script(Ref<Script> p_script);
};

String databag_validate_script(Ref<Script> p_script);

/// This is an utility that is intended for in editor usage that allow:
/// - Save script components.
/// - Load script components.
/// - Get script components properties.
/// - Save script databags.
/// - Load script databags.
/// - Save script systems.
/// - Load script systems.
class EditorEcs {
	static bool def_defined_static_components;
	/// Used to know if the previously stored `Component`s got loaded.
	static bool component_loaded;
	/// Used to know if the previously stored `System`s got loaded.
	static bool systems_loaded;
	/// Used to know if the ScriptedEcs components are registered.
	static bool ecs_initialized;

	/// List of script components each spawner can spawn.
	/// @Key: Spawner name
	/// @Value: List of script components
	static OAHashMap<StringName, Set<StringName>> spawners;

	static LocalVector<StringName> component_names;
	static LocalVector<Ref<Component>> components;

	static LocalVector<StringName> system_names;
	static LocalVector<Ref<System>> systems;

public:
	/// Clear the internal memory before the complete shutdown.
	static void __static_destructor();

	// ------------------------------------------------------------------ Spawner
	/// Returns the components name this spawner can spawn.
	static Vector<StringName> spawner_get_components(const StringName &spawner_name);

	// ---------------------------------------------------------------- Component
	/// Loads components.
	static void load_components();

	/// Load or Reloads a component. Retuns the component id.
	static StringName reload_component(const String &p_path);

	/// Returns all the scripted components.
	static const LocalVector<Ref<Component>> &get_components();

	/// Returns `true` if the given name points to a scripted component.
	static bool is_script_component(const StringName &p_name);

	/// Returns a script component.
	static Ref<Component> get_script_component(const StringName &p_name);

	static void component_get_properties(const StringName &p_component_name, List<PropertyInfo> *r_properties);
	static bool component_get_property_default_value(const StringName &p_component_name, const StringName &p_property_name, Variant &r_ret);
	static bool component_is_shared(const StringName &p_component_name);

	/// Store this script as component.
	/// Returns an empty string if succeede or the error message to disaply.
	/// Must be used in editor.
	static String component_save_script(const String &p_script_path, Ref<Script> p_script);

	// ------------------------------------------------------------------ Databag

	// ------------------------------------------------------------------- System

	/// Loads systems.
	static void load_systems();

	static StringName reload_system(const String &p_path);
	static bool is_script_system(const StringName &p_name);

	/// Store this script as System.
	/// Returns an empty string if succeede or the error message to disaply.
	/// Must be used in editor.
	static String system_save_script(const String &p_script_path, Ref<Script> p_script);

	// ------------------------------------------------------------------ Runtime

	static void define_editor_default_component_properties();

	static void register_runtime_scripts();
	static void register_dynamic_components();
	static void register_dynamic_component(Component *p_component);
	static void register_dynamic_systems();

private:
	static bool save_script(const String &p_setting_list_name, const String &p_script_path);
};
