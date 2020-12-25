/** @author AndreaCatania */

#pragma once

#include "../iterators/dynamic_query.h"
#include "core/templates/local_vector.h"
#include "system.h"

class World;

class Pipeline;

namespace godex {

class DynamicSystemInfo;

/// This function register the `DynamicSystemInfo` in a static array (generated
/// at compile time) and returns a pointer to a function that is able to call
/// `godex::DynamicSystemInfo::executor()` with the passed `DynamicSystemInfo`.
uint32_t register_dynamic_system(const DynamicSystemInfo &p_info);
get_system_exec_info_func get_dynamic_system_get_exec_info(uint32_t p_dynamic_system_id);
DynamicSystemInfo *get_dynamic_system_info(uint32_t p_dynamic_system_id);

/// `DynamicSystemInfo` is a class used to compose a system at runtime.
/// It's able to execute script systems and sub pipeline systems, in both case
/// the dependency graph is only known at runtime.
class DynamicSystemInfo {
	struct DResource {
		uint32_t resource_id;
		bool is_mutable;
	};

	Object *target_script = nullptr;
	Pipeline *target_sub_pipeline = nullptr;

	/// Map used to map the list of Resources to the script.
	LocalVector<uint32_t> resource_element_map;
	/// Map used to map the list of Components to the script.
	LocalVector<uint32_t> query_element_map;
	LocalVector<DResource> resources;
	DynamicQuery query;

public:
	DynamicSystemInfo();

	void set_target(Object *p_target);
	void set_target(Pipeline *p_pipeline);

	void with_resource(uint32_t p_resource_id, bool p_mutable);
	void with_component(uint32_t p_component_id, bool p_mutable);
	void without_component(uint32_t p_component_id);

public:
	static StringName for_each_name;
	// TODO instead to return the info consider pass it as reference, to avoid
	// any copy.
	static SystemExeInfo get_info(DynamicSystemInfo &p_info, system_execute p_exec);
	static void executor(World *p_world, DynamicSystemInfo &p_info);
};

} // namespace godex
