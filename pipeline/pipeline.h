#pragma once

/**
	@author AndreaCatania
*/

#include "../systems/system.h"
#include "core/templates/local_vector.h"

class World;

class Pipeline {
	bool ready = false;
	LocalVector<get_system_exec_info_func> systems_info;
	LocalVector<system_execute> systems;

public:
	Pipeline();

	/// Add a system that is registered via `ECS`, usually this function is used
	/// to construct the pipeline using the `ECS` class.
	/// This only difference with `add_system` is the argument type.
	void add_registered_system(uint32_t p_id);

	/// Add a system that is not registered via `ECS`.
	void add_system(get_system_exec_info_func p_get_info_func);

	/// Build the pipelines and makes it ready to dispatch.
	/// You can't modify it anymore, after calling this.
	void build();

	/// Returns `true` if the pipeline is ready.
	bool is_ready() const;

	/// get the systems dependencies. The same `Component` or `Resource` can
	/// be found as mutable and immutable.
	void get_systems_dependencies(SystemExeInfo &p_info) const;

	/// Reset the pipeline.
	void reset();

	// Dispatch the pipeline on the following world.
	void dispatch(World *p_world);
};

// This macro save the user the need to pass a `SystemExeInfo`, indeed it wraps
// the passed function with a labda function that creates a `SystemExeInfo`.
// By defining the same name of the method, the IDE autocomplete shows the method
// name `add_system`, properly + it's impossible use the function directly
// by mistake.
#define add_system(func)                                                      \
	add_system([]() -> SystemExeInfo {                                        \
		SystemExeInfo i = SystemBuilder::get_system_info_from_function(func); \
		i.system_func = [](World *p_world) {                                  \
			SystemBuilder::system_exec_func(p_world, func);                   \
		};                                                                    \
		return i;                                                             \
	})
