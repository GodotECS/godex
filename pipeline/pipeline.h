#pragma once

#include "../ecs.h"
#include "../systems/system.h"
#include "core/templates/local_vector.h"

class World;

struct ExecutionSystemData {
	godex::system_id id;
	func_system_execute exe;
};

struct ExecutionStageData {
	/// These systems can run in parallel.
	LocalVector<ExecutionSystemData> systems;

	/// Storages that want to be notified at the end of the `System` execution.
	LocalVector<godex::component_id> notify_list_release_write;
};

struct DispatcherData {
	LocalVector<ExecutionStageData> exec_stages;
};

class Pipeline {
	friend class PipelineBuilder;
	friend class ECS;

	bool ready = false;
	LocalVector<func_temporary_system_execute> temporary_systems_exe;

	LocalVector<DispatcherData> dispatchers;

public:
	Pipeline();

	/// Add a `TemporarySystem` which is executed untill `true` is returned.
	/// The `TemporarySystems` are always executed in single thread and before
	/// any other `System`.
	void add_temporary_system(func_temporary_system_execute p_func_get_exe_info);
	void add_registered_temporary_system(uint32_t p_id);

	/// Returns `true` if the pipeline is ready.
	bool is_ready() const;

	/// Reset the pipeline.
	void reset();

	/// Prepare the world to be safely dispatched.
	void prepare(World *p_world);

	/// Dispatch the pipeline on the following world.
	void dispatch(World *p_world);

private:
	void dispatch_sub_dispatcher(World *p_world, int p_dispatcher_idex);

public:
	/// Returns the stage index, or -1 if the system is not in pipeline.
	int get_system_stage(godex::system_id p_system, int p_start_from_dispatcher = 0) const;

	/// Returns the dispatcher id for this system.
	int get_system_dispatcher(godex::system_id p_system) const;
};

// This macro save the user the need to pass a `SystemExeInfo`, indeed it wraps
// the passed function with a labda function that creates a `SystemExeInfo`.
// By defining the same name of the method, the IDE autocomplete shows the method
// name `add_system`, properly + it's impossible use the function directly
// by mistake.
#define add_temporary_system(func)                                       \
	add_temporary_system([](World *p_world) -> bool {                    \
		return SystemBuilder::temporary_system_exec_func(p_world, func); \
	})
