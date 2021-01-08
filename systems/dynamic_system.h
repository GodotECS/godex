/** @author AndreaCatania */

#pragma once

#include "../iterators/dynamic_query.h"
#include "core/templates/local_vector.h"
#include "system.h"

class World;
class Pipeline;
class GDScriptFunction;

namespace godex {

class DynamicSystemInfo;

typedef void (*func_system_execute_pipeline)(World *p_world, Pipeline *p_pipeline);

/// This function register the `DynamicSystemInfo` in a static array (generated
/// at compile time) and returns a pointer to a function that is able to call
/// `godex::DynamicSystemInfo::executor()` with the passed `DynamicSystemInfo`.
uint32_t register_dynamic_system();
func_get_system_exe_info get_func_dynamic_system_exec_info(uint32_t p_dynamic_system_id);
DynamicSystemInfo *get_dynamic_system_info(uint32_t p_dynamic_system_id);

/// `DynamicSystemInfo` is a class used to compose a system at runtime.
/// It's able to execute script systems and sub pipeline systems, in both case
/// the dependency graph is only known at runtime.
// TODO this works, but I'm not really happy about it.
//      Please, split this class or improve the mechanism so that script query
//      and sub pipeline executor are not held by the same class at the same time.
class DynamicSystemInfo {
	// ~~ Script system ~~
	struct DDatabag {
		uint32_t databag_id;
		bool is_mutable;
	};

	ScriptInstance *target_script = nullptr;

	bool compiled = false;

	// Function direct access, for fast GDScript execution.
	GDScriptFunction *gdscript_function = nullptr;

	uint32_t system_id = UINT32_MAX;

	/// Map used to map the list of Databags to the script.
	LocalVector<uint32_t> databag_element_map;
	/// Map used to map the list of Components to the script.
	LocalVector<uint32_t> query_element_map;
	LocalVector<DDatabag> databags;
	DynamicQuery query;

	// Accessors.
	LocalVector<Variant> access;
	LocalVector<Variant *> access_ptr;

	// Accessors databag.
	LocalVector<DataAccessorScriptInstance<Databag> *> databag_accessors;
	LocalVector<Object> databag_accessors_obj;

	// ~~ Sub pipeline system ~~
	func_system_execute_pipeline sub_pipeline_execute = nullptr;
	Pipeline *target_sub_pipeline = nullptr;

public:
	DynamicSystemInfo();

	void set_system_id(uint32_t p_id);
	void set_target(ScriptInstance *p_target);

	void with_databag(uint32_t p_databag_id, bool p_mutable);
	void with_component(uint32_t p_component_id, bool p_mutable);
	void maybe_component(uint32_t p_component_id, bool p_mutable);
	void without_component(uint32_t p_component_id);

	void set_target(func_system_execute_pipeline p_sub_pipeline_execite);
	void set_pipeline(Pipeline *p_pipeline);

	bool build();

	bool is_system_dispatcher() const;

	EntityID get_current_entity_id() const;

public:
	static StringName for_each_name;

	static void get_info(DynamicSystemInfo &p_info, func_system_execute p_exec, SystemExeInfo &r_out);
	static void executor(World *p_world, DynamicSystemInfo &p_info);
};

} // namespace godex
