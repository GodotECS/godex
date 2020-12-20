#pragma once

/**
	@author AndreaCatania
*/

#include "core/templates/local_vector.h"
#include "modules/ecs/systems/system.h"

class World;

class Pipeline {
	LocalVector<system_execute> systems;

public:
	Pipeline();

	/// Add a system that is registered via `ECS`, usually this function is used
	/// to construct the pipeline using the `ECS` class.
	/// This only difference with `add_system` is the argument type.
	void add_registered_system(const SystemInfo &p_system_info);

	/// Add a system that is not registered via `ECS`.
	void add_system(get_system_info_func p_get_info_func);

	// Dispatch the pipeline on the following world.
	void dispatch(World *p_world);
};

// This macro save the user the need to pass a `SystemInfo`, indeed it wraps
// the passed function with a labda function that creates a `SystemInfo`.
// By defining the same name of the method, the IDE autocomplete shows the method
// name `add_system`, properly + it's impossible use the function directly
// by mistake.
#define add_system(func)                                                   \
	add_system([]() -> SystemInfo {                                        \
		SystemInfo i = SystemBuilder::get_system_info_from_function(func); \
		i.system_func = [](World *p_world) {                               \
			SystemBuilder::system_exec_func(p_world, func);                \
		};                                                                 \
		return i;                                                          \
	})
