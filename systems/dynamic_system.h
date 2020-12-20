/** @author AndreaCatania */

#pragma once

#include "core/templates/local_vector.h"
#include "modules/ecs/iterators/dynamic_query.h"
#include "system.h"

class World;

namespace godex {

/// This function register the `DynamicSystemInfo` in a static array (generated
/// at compile time) and returns a pointer to a function that is able to call
/// `godex::DynamicSystemInfo::executor()` with the passed `DynamicSystemInfo`.
system_execute register_dynamic_system(const DynamicSystemInfo &p_info);

class DynamicSystemInfo {
	struct DResource {
		uint32_t resource_id;
		bool is_mutable;
	};

	Object *target;
	/// Map used to map the list of Resources to the script.
	LocalVector<uint32_t> resource_element_map;
	/// Map used to map the list of Components to the script.
	LocalVector<uint32_t> query_element_map;
	LocalVector<DResource> resources;
	DynamicQuery query;

public:
	DynamicSystemInfo();

	void set_target(Object *p_target);
	void with_resource(uint32_t p_resource_id, bool p_mutable);
	void with_component(uint32_t p_component_id, bool p_mutable);
	void without_component(uint32_t p_component_id);

	SystemInfo get_system_info() const;

public:
	static StringName for_each_name;
	/// This function is called
	static void executor(World *p_world, DynamicSystemInfo &p_info);
};

} // namespace godex
