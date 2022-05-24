#include "pipeline.h"

#include "../ecs.h"
#include "../storage/hierarchical_storage.h"
#include "../world/world.h"
#include "pipeline_commands.h"

Pipeline::Pipeline() {}

bool Pipeline::is_ready() const {
	return ready;
}

bool Pipeline::can_change() const {
	for (uint32_t i = 0; i < worlds.size(); i += 1) {
		if (worlds[i].world != nullptr) {
			return false;
		}
	}
	return true;
}

void Pipeline::reset() {
	ready = false;
	temporary_systems.clear();
	dispatchers.clear();

	// Deallocate any valid token.
	for (uint32_t i = 0; i < worlds.size(); i += 1) {
		Token t;
		t.index = i;
		t.generation = worlds[i].generation;
		release_world(t);
	}
}

Token Pipeline::get_token(World *p_world) {
	Token token;

	// Phase 1: Verify if this pipeline has prepared this World already.
	for (uint32_t i = 0; i < worlds.size(); i += 1) {
		if (worlds[i].world == p_world) {
			// This world is already initialized, nothing to do, just return the token.
			token.index = i;
			token.generation = worlds[i].generation;
			break;
		}
	}

	return token;
}

Token Pipeline::prepare_world(World *p_world) {
	// Phase 1: Verify if this pipeline has prepared this World already.
	Token token = get_token(p_world);
	if (token.is_valid()) {
		// This token already exists, so the world was already prepared.
		// Just return it.
		return token;
	}

	// Phase 2: Prepare the World.
	p_world->create_databag<PipelineCommands>();
	token.index = 0;
	token.generation = 0;

	ERR_FAIL_COND_V_MSG(!is_ready(), token, "The Pipeline is not ready, you can't prepare a world at this point.");

	for (uint32_t i = 0; i < worlds.size(); i += 1) {
		if (worlds[i].world == nullptr) {
			// We have a free space, let's use it.
			token.index = i;
			token.generation = worlds[i].generation;
			break;
		}
	}

	if (token.is_valid() == false) {
		token.index = worlds.size();
		token.generation = 1;
		worlds.push_back(WorldData());
	}

	worlds[token.index].world = p_world;
	worlds[token.index].active = false;

	// Phase 3: Crete components and databags storages.
	SystemExeInfo info;

	for (uint32_t dispatcher_i = 0; dispatcher_i < dispatchers.size(); dispatcher_i += 1) {
		DispatcherData &dispatcher = dispatchers[dispatcher_i];

		for (uint32_t stage_i = 0; stage_i < dispatcher.exec_stages.size(); stage_i += 1) {
			for (uint32_t i = 0; i < dispatcher.exec_stages[stage_i].systems.size(); i += 1) {
				info.clear();
				const godex::system_id id = dispatcher.exec_stages[stage_i].systems[i].id;
				ECS::get_system_exe_info(id, info);
				create_used_storage(info, p_world);
			}
		}
	}

	// Make sure to init the storage also for temporary systems.
	for (uint32_t i = 0; i < temporary_systems.size(); i += 1) {
		info.clear();
		ECS::get_system_exe_info(temporary_systems[i], info);
		create_used_storage(info, p_world);
	}

	// Phase 4: Allocate SystemExecutionData memory.
	// At this point we need to allocate the SystemData, which is a struct where
	// the system can store some information about the execution.
	// For example, the `Query` will cache some vectors, to speedup the executions.

	{
		worlds[token.index].system_data_buffer_size = 0;
		for (uint32_t dispatcher_i = 0; dispatcher_i < dispatchers.size(); dispatcher_i += 1) {
			DispatcherData &dispatcher = dispatchers[dispatcher_i];

			for (uint32_t stage_i = 0; stage_i < dispatcher.exec_stages.size(); stage_i += 1) {
				for (uint32_t i = 0; i < dispatcher.exec_stages[stage_i].systems.size(); i += 1) {
					const godex::system_id id = dispatcher.exec_stages[stage_i].systems[i].id;
					worlds[token.index].system_data_buffer_size += ECS::system_get_size_system_data(id);
				}
			}
		}

		worlds[token.index].system_data_buffer = static_cast<uint8_t *>(memalloc(worlds[token.index].system_data_buffer_size));
	}

	// Phase 5: Initialize the SystemExecutionData pointers.
	{
		uint64_t offset = 0;
		for (uint32_t dispatcher_i = 0; dispatcher_i < dispatchers.size(); dispatcher_i += 1) {
			DispatcherData &dispatcher = dispatchers[dispatcher_i];

			for (uint32_t stage_i = 0; stage_i < dispatcher.exec_stages.size(); stage_i += 1) {
				for (uint32_t i = 0; i < dispatcher.exec_stages[stage_i].systems.size(); i += 1) {
					// This is impossible to be triggered, since the `PipelineBuilder`
					// uses this exact algorithm to define the `System` index.
					// By checking this, I'm double checking that the system `index`
					// points to the correct memory location.
					CRASH_COND(worlds[token.index].system_data.size() != dispatcher.exec_stages[stage_i].systems[i].index);

					const godex::system_id id = dispatcher.exec_stages[stage_i].systems[i].id;

					// Take the SystemExecutionData pointer allocated for this system.
					uint8_t *system_data_ptr = worlds[token.index].system_data_buffer + offset;
					// Store it for fast access.
					worlds[token.index].system_data.push_back(system_data_ptr);
					// Initialize.
					ECS::system_new_placement_system_data(id, system_data_ptr, token, p_world, this);

					// Set the system as deactivated.
					ECS::system_set_active_system(id, system_data_ptr, false);

					// Advance the offset by the used size.
					const uint64_t system_data_size = ECS::system_get_size_system_data(id);
					offset += system_data_size;
				}
			}
		}
	}

	// Initialize the temporary systems.
	for (uint32_t i = 0; i < temporary_systems.size(); i += 1) {
		const uint64_t size = ECS::system_get_size_system_data(temporary_systems[i]);
		uint8_t *mem = (uint8_t *)memalloc(size);
		ECS::system_new_placement_system_data(temporary_systems[i], mem, token, p_world, this);

		TemporaryExecutionSystemData d;
		d.id = temporary_systems[i];
		d.exec_func = ECS::get_func_temporary_system_exe(temporary_systems[i]);
		d.system_data = mem;
		worlds[token.index].temporary_systems.push_back(d);

		// Set the system as deactivated.
		ECS::system_set_active_system(d.id, mem, false);
	}

	p_world->associated_pipelines.push_back(this);

	return token;
}

void Pipeline::create_used_storage(const SystemExeInfo &p_info, World *p_world) {
	// Create components.
	for (const RBSet<uint32_t>::Element *e = p_info.immutable_components.front(); e; e = e->next()) {
		p_world->create_storage(e->get());
	}

	for (const RBSet<uint32_t>::Element *e = p_info.mutable_components.front(); e; e = e->next()) {
		p_world->create_storage(e->get());
	}

	for (const RBSet<uint32_t>::Element *e = p_info.mutable_components_storage.front(); e; e = e->next()) {
		p_world->create_storage(e->get());
	}

	// Create databags.
	for (const RBSet<uint32_t>::Element *e = p_info.immutable_databags.front(); e; e = e->next()) {
		p_world->create_databag(e->get());
	}

	for (const RBSet<uint32_t>::Element *e = p_info.mutable_databags.front(); e; e = e->next()) {
		p_world->create_databag(e->get());
	}

	for (const RBSet<uint32_t>::Element *e = p_info.events_emitters.front(); e; e = e->next()) {
		p_world->create_events_storage(e->get());
	}

	for (
			OAHashMap<uint32_t, RBSet<String>>::Iterator it = p_info.events_receivers.iter();
			it.valid;
			it = p_info.events_receivers.next_iter(it)) {
		p_world->create_events_storage(*it.key);
		EventStorageBase *s = p_world->get_events_storage(*it.key);
		for (const RBSet<String>::Element *e = it.value->front(); e; e = e->next()) {
			s->add_event_emitter(e->get());
		}
	}
}

void Pipeline::release_world(Token p_token) {
	ERR_FAIL_COND_MSG(p_token.is_valid() == false, "The passed token is invalid.");
	ERR_FAIL_UNSIGNED_INDEX_MSG(p_token.index, worlds.size(), "The token: " + itos(p_token.index) + " is not used.");
	ERR_FAIL_COND_MSG(worlds[p_token.index].world == nullptr, "The token index: `" + itos(p_token.index) + "` is not used.");
	ERR_FAIL_COND_MSG(p_token.generation != worlds[p_token.index].generation, "The token generation: `" + itos(p_token.generation) + "` is different from the world generation `" + itos(p_token.generation) + "`. Maybe it's an old Token?");

	{
		const LocalVector<uint8_t *> &system_data_ptrs = worlds[p_token.index].system_data;

		for (uint32_t dispatcher_i = 0; dispatcher_i < dispatchers.size(); dispatcher_i += 1) {
			DispatcherData &dispatcher = dispatchers[dispatcher_i];

			for (uint32_t stage_i = 0; stage_i < dispatcher.exec_stages.size(); stage_i += 1) {
				for (uint32_t i = 0; i < dispatcher.exec_stages[stage_i].systems.size(); i += 1) {
					const godex::system_id id = dispatcher.exec_stages[stage_i].systems[i].id;
					const uint32_t index = dispatcher.exec_stages[stage_i].systems[i].index;
					ECS::system_delete_placement_system_data(id, system_data_ptrs[index]);
				}
			}
		}

		memfree(worlds[p_token.index].system_data_buffer);
	}

	// Clear temporary system data
	{
		const LocalVector<TemporaryExecutionSystemData> &temp_systems = worlds[p_token.index].temporary_systems;
		for (uint32_t i = 0; i < temp_systems.size(); i += 1) {
			ECS::system_delete_placement_system_data(temp_systems[i].id, temp_systems[i].system_data);
			memfree(temp_systems[i].system_data);
		}
	}

	// Just reset this, we will reuse this memory eventually.
	worlds[p_token.index].system_data_buffer = nullptr;
	worlds[p_token.index].system_data_buffer_size = 0;
	worlds[p_token.index].system_data.reset();
	worlds[p_token.index].temporary_systems.reset();
	worlds[p_token.index].world = nullptr;

	// Increase the generation by 1, so any eventual old token still saved
	// somewhere is invalidated.
	worlds[p_token.index].generation += 1;
}

void Pipeline::set_active(Token p_token, bool p_active) {
	ERR_FAIL_COND_MSG(p_token.is_valid() == false, "The passed token is invalid.");
	ERR_FAIL_UNSIGNED_INDEX_MSG(p_token.index, worlds.size(), "The token: " + itos(p_token.index) + " is not used.");
	ERR_FAIL_COND_MSG(worlds[p_token.index].world == nullptr, "The token index: `" + itos(p_token.index) + "` is not used.");
	ERR_FAIL_COND_MSG(p_token.generation != worlds[p_token.index].generation, "The token generation: `" + itos(p_token.generation) + "` is different from the world generation `" + itos(p_token.generation) + "`. Maybe it's an old Token?");

	if (worlds[p_token.index].active == p_active) {
		// Nothing to do.
		return;
	}

	const LocalVector<uint8_t *> &system_data_ptrs = worlds[p_token.index].system_data;

	for (uint32_t dispatcher_i = 0; dispatcher_i < dispatchers.size(); dispatcher_i += 1) {
		DispatcherData &dispatcher = dispatchers[dispatcher_i];

		for (uint32_t stage_i = 0; stage_i < dispatcher.exec_stages.size(); stage_i += 1) {
			for (uint32_t i = 0; i < dispatcher.exec_stages[stage_i].systems.size(); i += 1) {
				const godex::system_id id = dispatcher.exec_stages[stage_i].systems[i].id;
				const uint32_t index = dispatcher.exec_stages[stage_i].systems[i].index;
				ECS::system_set_active_system(id, system_data_ptrs[index], p_active);
			}
		}
	}

	// Set the system as deactivated.
	const LocalVector<TemporaryExecutionSystemData> &temp_system_data = worlds[p_token.index].temporary_systems;
	for (uint32_t i = 0; i < temp_system_data.size(); i += 1) {
		ECS::system_set_active_system(
				temp_system_data[i].id,
				temp_system_data[i].system_data,
				p_active);
	}

	worlds[p_token.index].active = p_active;
}

void Pipeline::dispatch(Token p_token) {
#ifdef DEBUG_ENABLED
	CRASH_COND_MSG(ready == false, "You can't dispatch a pipeline which is not yet builded. Please call `build`.");
#endif

	ERR_FAIL_COND_MSG(p_token.is_valid() == false, "The passed token is invalid.");
	ERR_FAIL_UNSIGNED_INDEX_MSG(p_token.index, worlds.size(), "The token: " + itos(p_token.index) + " is not used.");
	ERR_FAIL_COND_MSG(worlds[p_token.index].world == nullptr, "The token index: `" + itos(p_token.index) + "` is not used.");
	ERR_FAIL_COND_MSG(p_token.generation != worlds[p_token.index].generation, "The token generation: `" + itos(p_token.generation) + "` is different from the world generation `" + itos(p_token.generation) + "`. Maybe it's an old Token?");
	ERR_FAIL_COND_MSG(worlds[p_token.index].active == false, "The world pointed by the token index: `" + itos(p_token.index) + "` is not active. You need to activate the world so process it.");

	World *world = worlds[p_token.index].world;
	ERR_FAIL_COND_MSG(world->is_dispatching_in_progress, "Dispatching is already in progress for this world. Only one pipeline is allowed to be dispatched on a world.");

	// Prepare the world for dispatching.
	world->is_dispatching_in_progress = true;
	PipelineCommands *pipeline_commands = world->get_databag<PipelineCommands>();
#ifdef DEBUG_ENABLED
	CRASH_COND_MSG(pipeline_commands == nullptr, "The PipelineCommands is never expected to be nullptr, since this class make sure to add it on this world.");
#endif
	pipeline_commands->world_data = worlds.ptr() + p_token.index;
	pipeline_commands->pipeline = this;

	Hierarchy *hierarchy = static_cast<Hierarchy *>(world->get_storage<Child>());
	if (hierarchy) {
		// Flush the hierarchy.
		hierarchy->flush_hierarchy_changes();
	}

	// Process the `TemporarySystem`, if any.
	for (int i = 0; i < int(worlds[p_token.index].temporary_systems.size()); i += 1) {
		if (worlds[p_token.index].temporary_systems[i].exec_func(
					worlds[p_token.index].temporary_systems[i].system_data,
					world)) {
			// This system is done, deallocate.
			uint8_t *mem = worlds[p_token.index].temporary_systems[i].system_data;
			ECS::system_delete_placement_system_data(worlds[p_token.index].temporary_systems[i].id, mem);
			memfree(mem);
			worlds[p_token.index].temporary_systems.remove_at(i);
			i -= 1;
		}
	}

	dispatch_sub_dispatcher(p_token, 0);

	// TODO remove this, in favour of the new mechanism
	// Flush changed.
	for (uint32_t c = 0; c < world->storages.size(); c += 1) {
		if (world->storages[c] != nullptr) {
			world->storages[c]->flush_changed();
		}
	}

	// Release the world dispatching.
	pipeline_commands->world_data = nullptr;
	pipeline_commands->pipeline = nullptr;
	world->is_dispatching_in_progress = false;
}

void Pipeline::dispatch_sub_dispatcher(Token p_token, int p_dispatcher_index) {
	ERR_FAIL_INDEX_MSG(p_dispatcher_index, int(dispatchers.size()), "The dispatcher " + itos(p_dispatcher_index) + " doesn't exists in this pipeline. This is a bug, please report it.");

	World *world = worlds[p_token.index].world;
	const LocalVector<uint8_t *> &system_data_ptrs = worlds[p_token.index].system_data;
	const DispatcherData &dispatcher = dispatchers[p_dispatcher_index];

	// Dispatch the `Stage`s.
	for (uint32_t stage_i = 0; stage_i < dispatcher.exec_stages.size(); stage_i += 1) {
		// TODO execute in multuple thread.
		for (uint32_t i = 0; i < dispatcher.exec_stages[stage_i].systems.size(); i += 1) {
			const uint32_t index = dispatcher.exec_stages[stage_i].systems[i].index;
			dispatcher.exec_stages[stage_i].systems[i].exe(
					system_data_ptrs[index],
					world);
		}

		// TODO move this inside the DataFetcher instead?
		// Notify the `System` released the storage for this stage.
		for (uint32_t f = 0; f < dispatcher.exec_stages[stage_i].notify_list_release_write.size(); f += 1) {
			world->get_storage(dispatcher.exec_stages[stage_i].notify_list_release_write[f])->on_system_release();
		}
	}
}

int Pipeline::get_system_stage(godex::system_id p_system, int p_start_from_dispatcher) const {
	ERR_FAIL_INDEX_V_MSG(p_start_from_dispatcher, int(dispatchers.size()), -1, "The dispatcher " + itos(p_start_from_dispatcher) + " doesn't exists in this pipeline.");

	int stage_index = 0;
	for (uint32_t dispatcher_i = p_start_from_dispatcher; dispatcher_i < dispatchers.size(); dispatcher_i += 1) {
		const DispatcherData &dispatcher = dispatchers[dispatcher_i];

		for (uint32_t stage_i = 0; stage_i < dispatcher.exec_stages.size(); stage_i += 1) {
			for (uint32_t i = 0; i < dispatcher.exec_stages[stage_i].systems.size(); i += 1) {
				if (dispatcher.exec_stages[stage_i].systems[i].id == p_system) {
					return stage_index;
				}
			}
			stage_index += 1;
		}
	}
	return -1;
}

int Pipeline::get_system_dispatcher(godex::system_id p_system) const {
	for (uint32_t dispatcher_i = 0; dispatcher_i < dispatchers.size(); dispatcher_i += 1) {
		const DispatcherData &dispatcher = dispatchers[dispatcher_i];

		for (uint32_t stage_i = 0; stage_i < dispatcher.exec_stages.size(); stage_i += 1) {
			for (uint32_t i = 0; i < dispatcher.exec_stages[stage_i].systems.size(); i += 1) {
				if (dispatcher.exec_stages[stage_i].systems[i].id == p_system) {
					return dispatcher_i;
				}
			}
		}
	}
	return -1;
}
