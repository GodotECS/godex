#pragma once

#include "../ecs.h"
#include "../systems/system.h"
#include "core/templates/local_vector.h"

class World;

struct TemporaryExecutionSystemData {
	// The SystemID
	godex::system_id id;
	func_temporary_system_execute exec_func;
	uint8_t *system_data;
};

struct ExecutionSystemData {
	// The SystemID
	godex::system_id id;
	// The System index within this pipeline. This is used to fetch the
	// SystemExecutionData, created by the `prepare_world` function.
	uint32_t index;
	// The Execution function.
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

struct WorldData {
	friend class Pipeline;
	friend class PipelineCommands;

private:
	uint8_t generation = 1;
	World *world;
	bool active;

	uint64_t system_data_buffer_size;
	uint8_t *system_data_buffer;

	// Pointers to the system data.
	LocalVector<uint8_t *> system_data;

	LocalVector<TemporaryExecutionSystemData> temporary_systems;
};

class Pipeline {
	friend class PipelineBuilder;
	friend class ECS;
	friend class PipelineCommands;

private:
	bool ready = false;
	LocalVector<godex::system_id> temporary_systems;

	LocalVector<DispatcherData> dispatchers;

	/// List of worlds ready to be dispatched by this pipeline.
	LocalVector<WorldData> worlds;

public:
	Pipeline();

	/// Returns false if this pipeline has some bind world, that it's necessary to
	/// release before altering this pipeline.
	bool can_change() const;

	/// Returns `true` if the pipeline is ready.
	bool is_ready() const;

	/// Reset the pipeline.
	void reset();

	Token get_token(World *p_world);

	/// Prepare the world to be safely dispatched, returns a token to use to
	/// dispatch the World.
	Token prepare_world(World *p_world);

private:
	void create_used_storage(const SystemExeInfo &p_info, World *p_world);

public:
	/// Releases the token created by `prepare`.
	void release_world(Token p_token);

	/// Activate or Deactivate this pipeline for the given token.
	/// Activate the pipeline just before dispatching.
	void set_active(Token p_token, bool p_active);

	/// Dispatch the pipeline on the following world.
	void dispatch(Token p_token);

private:
	void dispatch_sub_dispatcher(Token p_token, int p_dispatcher_idex);

public:
	/// Returns the stage index, or -1 if the system is not in pipeline.
	int get_system_stage(godex::system_id p_system, int p_start_from_dispatcher = 0) const;

	/// Returns the dispatcher id for this system.
	int get_system_dispatcher(godex::system_id p_system) const;
};
