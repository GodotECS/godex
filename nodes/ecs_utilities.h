#pragma once

#include "core/io/resource.h"
#include "core/templates/local_vector.h"
#include "core/templates/oa_hash_map.h"
#include "modules/ecs/ecs.h"

class Script;

namespace godex {
class DynamicSystemInfo;
}

class System : public Resource {
	GDCLASS(System, Resource);

	friend class ScriptECS;

	godex::DynamicSystemInfo *info = nullptr;
	godex::system_id id = UINT32_MAX;

	static void _bind_methods();
	void prepare(godex::DynamicSystemInfo *p_info);

public:
	enum Mutability {
		IMMUTABLE,
		MUTABLE
	};

public:
	System();
	~System();

	void with_resource(const StringName &p_resource_name, Mutability p_mutability);
	void with_component(const StringName &p_component_name, Mutability p_mutability);
	void without_component(const StringName &p_component_name);

	godex::system_id get_id() const;

	static String validate_script(Ref<Script> p_script);
};

VARIANT_ENUM_CAST(System::Mutability);

class Component : public Resource {
	GDCLASS(Component, Resource);

	friend class ScriptECS;

	StringName name;
	Ref<Script> component_script;

	static void _bind_methods();

	void set_name(StringName p_name);
	void set_component_script(Ref<Script> p_script);

public:
	Component();
	~Component();

	StringName get_name() const;

	void get_component_property_list(List<PropertyInfo> *p_info);
	Variant get_property_default_value(StringName p_property_name);

	/// Validate the script and returns a void string if the validation success
	/// or the error message.
	static String validate_script(Ref<Script> p_script);
};

String resource_validate_script(Ref<Script> p_script);

/// Utility that allow to handle the godot scripted Component, Resources, Systems.
class ScriptECS {
	static bool component_loaded;
	static bool ecs_initialized;

	static LocalVector<StringName> component_names;
	static LocalVector<Ref<Component>> components;

	static LocalVector<StringName> system_names;
	static LocalVector<Ref<System>> systems;

public:
	/// Loads components.
	static void load_components();

	/// Load or Reloads a component. Retuns the component id.
	static uint32_t reload_component(const String &p_path);

	static uint32_t get_component_id(const StringName &p_name);

	static const LocalVector<Ref<Component>> &get_components();

	static Ref<Component> get_component(uint32_t p_id);

	static void register_runtime_scripts();
	static void register_dynamic_components();

	static uint32_t reload_system(const String &p_path);
	static uint32_t get_system_id(const StringName &p_name);
	static void register_dynamic_systems();
};
