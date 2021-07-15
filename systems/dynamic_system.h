#pragma once

#include "../iterators/dynamic_query.h"
#include "core/templates/local_vector.h"
#include "system.h"

class World;
class Pipeline;
class GDScriptFunction;

namespace godex {

class DynamicSystemInfo;

/// This function register the `DynamicSystemInfo` in a static array (generated
/// at compile time) and returns a pointer to a function that is able to call
/// `godex::DynamicSystemInfo::executor()` with the passed `DynamicSystemInfo`.
uint32_t register_dynamic_system();
func_get_system_exe_info get_func_dynamic_system_exec_info(uint32_t p_dynamic_system_id);
DynamicSystemInfo *get_dynamic_system_info(uint32_t p_dynamic_system_id);
void __dynamic_system_info_static_destructor();

/// `DynamicSystemInfo` is a class used to compose a system at runtime.
/// It's able to execute script systems.
//
// Notice:
// This class is used by the `System` resource. Everything was implemented here
// instead to implement it directly in the `System` resource so godex can deal
// with non complex Godot pointer logic.
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

	// List of world fetchers.
	LocalVector<GodexWorldFetcher *> fetchers;

	// Parsed exposer
	LocalVector<Variant> access;
	LocalVector<Variant *> access_ptr;

public:
	DynamicSystemInfo();
	~DynamicSystemInfo();

	void set_system_id(uint32_t p_id);
	void set_target(ScriptInstance *p_target);

	void execute_in(Phase p_phase, const StringName &p_dispatcher_name = StringName());
	void execute_after(const StringName &p_system_name);
	void execute_before(const StringName &p_system_name);

	void with_query(DynamicQuery *p_query);
	void with_databag(uint32_t p_databag_id, bool p_mutable);
	void with_storage(godex::component_id p_component_id);
	void with_events_emitter(godex::event_id p_event_id);
	void with_events_receiver(godex::event_id p_event_id, const String &p_emitter_name);

	bool build();
	void reset();

public:
	static StringName execute_func_name;

	static void get_info(DynamicSystemInfo &p_info, func_system_execute p_exec, SystemExeInfo &r_out);
	static void executor(World *p_world, DynamicSystemInfo &p_info);
};
} // namespace godex
