#pragma once

#include "../../../ecs.h"
#include "core/io/resource.h" // TODO remove this or the one on the cpp?
#include "core/templates/local_vector.h"
#include "core/templates/oa_hash_map.h"

class Script;
class SharedComponentResource;

namespace godex {
class DynamicSystemInfo;
}

class System : public Resource {
	GDCLASS(System, Resource)

	friend class ScriptEcs;

	// Used by the editor to know if the associated script exists.
	bool verified = false;
	String script_path;
	godex::DynamicSystemInfo *info = nullptr;
	godex::system_id id = UINT32_MAX;
	bool prepare_in_progress = false;

	static void _bind_methods();
	const String &get_script_path() const;

public:
	enum Mutability {
		IMMUTABLE,
		MUTABLE
	};

public:
	System();
	~System();

	void execute_in(Phase p_phase, uint32_t p_dispatcher_id = godex::SYSTEM_NONE);
	void execute_after(uint32_t p_system);
	void execute_before(uint32_t p_system);

	void set_space(Space p_space);
	void with_databag(uint32_t p_databag_id, Mutability p_mutability);
	void with_storage(uint32_t p_component_id);
	void with_component(uint32_t p_component_id, Mutability p_mutability);
	void maybe_component(uint32_t p_component_id, Mutability p_mutability);
	void changed_component(uint32_t p_component_id, Mutability p_mutability);
	void not_component(uint32_t p_component_id);

	godex::system_id get_system_id() const;

	uint32_t get_current_entity_id() const;

	/// This function is only used by few tests. Never use this, use `prepare` instead.
	// TODO remove this function.
	void __force_set_system_info(godex::DynamicSystemInfo *p_info, godex::system_id p_id);
	/// Prepare this System to be executed.
	void prepare(godex::DynamicSystemInfo *p_info, godex::system_id p_id);

	static String validate_script(Ref<Script> p_script);
};

class SystemBundle : public Resource {
	GDCLASS(SystemBundle, Resource)

	friend class ScriptEcs;

	bool verified = false;
	String script_path;
	StringName name;

	static void _bind_methods();
	void __prepare();
	const String &get_script_path() const;

public:
	void add(uint32_t p_system_id);
	void with_description(const String &p_desc);
	void execute_before(uint32_t p_system_id);
	void execute_after(uint32_t p_system_id);

	static String validate_script(Ref<Script> p_script);
};

VARIANT_ENUM_CAST(System::Mutability)

class Component : public Resource {
	GDCLASS(Component, Resource)

	friend class ScriptEcs;

	// Used by the editor to know if the associated script exists.
	bool verified = false;
	String script_path;
	StringName name;

	static void _bind_methods();
	const String &get_script_path() const;

public:
	Component();
	~Component();

	void internal_set_name(StringName p_name);

	StringName get_name() const;

	void get_component_property_list(List<PropertyInfo> *p_info);
	Variant get_property_default_value(StringName p_property_name);

	Vector<StringName> get_spawners();

	/// Validate the script and returns a void string if the validation success
	/// or the error message.
	static String validate_script(Ref<Script> p_script);
};

String databag_validate_script(Ref<Script> p_script);

/// Used by the Entity3D & Entity2D to store the entity variables, when the
/// entity is not inside a world.
class ComponentDepot : public RefCounted {
	friend class ScriptEcs;

protected:
	StringName component_name;

public:
	virtual ~ComponentDepot();

	virtual void init(const StringName &p_name) = 0;
	virtual Dictionary get_properties_data() const = 0;
};

class StaticComponentDepot : public ComponentDepot {
	void *component = nullptr;
	godex::component_id component_id = godex::COMPONENT_NONE;

public:
	godex::component_id get_component_id() const { return component_id; }

	virtual ~StaticComponentDepot();

	virtual void init(const StringName &p_name) override;

	virtual bool _setv(const StringName &p_name, const Variant &p_value) override;
	virtual bool _getv(const StringName &p_name, Variant &r_ret) const override;

	virtual Dictionary get_properties_data() const override;
};

class ScriptComponentDepot : public ComponentDepot {
	Dictionary data;

public:
	virtual void init(const StringName &p_name) override;

	virtual bool _setv(const StringName &p_name, const Variant &p_value) override;
	virtual bool _getv(const StringName &p_name, Variant &r_ret) const override;

	virtual Dictionary get_properties_data() const override;
};

class SharedComponentDepot : public ComponentDepot {
	Ref<SharedComponentResource> data;

public:
	virtual void init(const StringName &p_name) override;

	virtual bool _setv(const StringName &p_name, const Variant &p_value) override;
	virtual bool _getv(const StringName &p_name, Variant &r_ret) const override;

	virtual Dictionary get_properties_data() const override;
};
