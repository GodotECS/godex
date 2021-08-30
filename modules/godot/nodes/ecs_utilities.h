#pragma once

#include "../../../ecs.h"
#include "core/io/resource.h" // TODO remove this or the one on the cpp?
#include "core/templates/local_vector.h"
#include "core/templates/oa_hash_map.h"

class Script;
class SharedComponentResource;

namespace godex {
class DynamicSystemExecutionData;
class DynamicQuery;
} // namespace godex

class System : public Resource {
	GDCLASS(System, Resource)

	friend class ScriptEcs;

	godex::system_id id;
	// Used by the editor to know if the associated script exists.
	bool verified = false;
	String script_path;
	godex::DynamicSystemExecutionData *info = nullptr;

	static void _bind_methods();
	const String &get_script_path() const;

public:
	enum Mutability {
		IMMUTABLE = 0,
		MUTABLE = 1
	};

public:
	System();
	~System();

	void execute_in(Phase p_phase, uint32_t p_dispatcher_id = godex::SYSTEM_NONE);
	void execute_after(uint32_t p_system);
	void execute_before(uint32_t p_system);

	void with_query_gd(Object *p_query);
	void with_query(godex::DynamicQuery *p_query);
	void with_databag(uint32_t p_databag_id, Mutability p_mutability);
	void with_storage(uint32_t p_component_id);
	void with_events_emitter(godex::event_id p_event_id);
	void with_events_receiver(godex::event_id p_event_id, const String &p_emitter_name);

	godex::system_id get_system_id() const;

	/// This function is only used by few tests. Never use this, use `prepare` instead.
	/// Prepare this System to be executed.
	void prepare(godex::DynamicSystemExecutionData *p_info);

	static String validate_script(Ref<Script> p_script);

	static void get_system_exec_info(godex::system_id p_id, SystemExeInfo &r_info);

	static uint64_t dynamic_system_data_get_size();
	static void dynamic_system_data_new_placement(uint8_t *r_mem, Token p_token, World *p_world, Pipeline *p_pipline, godex::system_id p_id);
	static void dynamic_system_data_delete_placement(uint8_t *p_mem);
	static void dynamic_system_data_set_active(uint8_t *p_mem, bool p_active);
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

	virtual void _get_property_list(List<PropertyInfo> *r_list) const = 0;
};

class StaticComponentDepot : public ComponentDepot {
	friend class SharedComponentResource;
	void *component = nullptr;
	godex::component_id component_id = godex::COMPONENT_NONE;

public:
	godex::component_id get_component_id() const { return component_id; }

	virtual ~StaticComponentDepot();

	virtual void init(const StringName &p_name) override;

	virtual bool _setv(const StringName &p_name, const Variant &p_value) override;
	virtual bool _getv(const StringName &p_name, Variant &r_ret) const override;

	virtual Dictionary get_properties_data() const override;
	virtual void _get_property_list(List<PropertyInfo> *r_list) const;
};

class ScriptComponentDepot : public ComponentDepot {
	Dictionary data;

public:
	virtual void init(const StringName &p_name) override;

	virtual bool _setv(const StringName &p_name, const Variant &p_value) override;
	virtual bool _getv(const StringName &p_name, Variant &r_ret) const override;

	virtual Dictionary get_properties_data() const override;
	virtual void _get_property_list(List<PropertyInfo> *r_list) const;
};

class SharedComponentDepot : public ComponentDepot {
	Ref<SharedComponentResource> data;

public:
	virtual void init(const StringName &p_name) override;

	virtual bool _setv(const StringName &p_name, const Variant &p_value) override;
	virtual bool _getv(const StringName &p_name, Variant &r_ret) const override;

	virtual Dictionary get_properties_data() const override;
	virtual void _get_property_list(List<PropertyInfo> *r_list) const;
};
