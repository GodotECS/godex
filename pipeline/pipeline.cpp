#include "pipeline.h"

#include "../ecs.h"
#include "../storage/hierarchical_storage.h"
#include "../world/world.h"

Pipeline::Pipeline() {
}

// Unset the macro defined into the `pipeline.h` so to properly point the method
// definition.
#undef add_temporary_system
void Pipeline::add_temporary_system(func_temporary_system_execute p_func_get_exe_info) {
	temporary_systems_exe.push_back(p_func_get_exe_info);
}

void Pipeline::add_registered_temporary_system(godex::system_id p_id) {
	add_temporary_system(ECS::get_func_temporary_system_exe(p_id));
}

bool Pipeline::is_ready() const {
	return ready;
}

void Pipeline::reset() {
	ready = false;
	temporary_systems_exe.clear();
	dispatchers.clear();
}

void Pipeline::prepare(World *p_world) {
	// Make sure to reset the `need_changed` for the storages of this world.
	for (uint32_t i = 0; i < p_world->storages.size(); i += 1) {
		if (p_world->storages[i] != nullptr) {
			p_world->storages[i]->reset_changed();
			p_world->storages[i]->set_tracing_change(false);
		}
	}

	// Crete components and databags storages.
	SystemExeInfo info;

	for (uint32_t dispatcher_i = 0; dispatcher_i < dispatchers.size(); dispatcher_i += 1) {
		DispatcherData &dispatcher = dispatchers[dispatcher_i];

		for (uint32_t stage_i = 0; stage_i < dispatcher.exec_stages.size(); stage_i += 1) {
			for (uint32_t i = 0; i < dispatcher.exec_stages[stage_i].systems.size(); i += 1) {
				info.clear();
				ECS::get_system_exe_info(dispatcher.exec_stages[stage_i].systems[i].id, info);

				// Create components.
				for (const Set<uint32_t>::Element *e = info.immutable_components.front(); e; e = e->next()) {
					p_world->create_storage(e->get());
				}

				for (const Set<uint32_t>::Element *e = info.mutable_components.front(); e; e = e->next()) {
					p_world->create_storage(e->get());
				}

				for (const Set<uint32_t>::Element *e = info.mutable_components_storage.front(); e; e = e->next()) {
					p_world->create_storage(e->get());
				}

				// Create databags.
				for (const Set<uint32_t>::Element *e = info.immutable_databags.front(); e; e = e->next()) {
					p_world->create_databag(e->get());
				}

				for (const Set<uint32_t>::Element *e = info.mutable_databags.front(); e; e = e->next()) {
					p_world->create_databag(e->get());
				}

				for (const Set<uint32_t>::Element *e = info.need_changed.front(); e; e = e->next()) {
					// Mark as `need_changed` this storage.
					StorageBase *storage = p_world->get_storage(e->get());
					ERR_CONTINUE_MSG(storage == nullptr, "The storage is not supposed to be nullptr at this point. Storage: " + ECS::get_component_name(e->get()) + "#" + itos(e->get()));
					storage->set_tracing_change(true);
				}
			}
		}
	}

	// Set the current `Components` as changed.
	for (uint32_t i = 0; i < p_world->storages.size(); i += 1) {
		if (p_world->storages[i] != nullptr) {
			if (p_world->storages[i]->is_tracing_change()) {
				const EntitiesBuffer entities = p_world->storages[i]->get_stored_entities();
				for (uint32_t e = 0; e < entities.count; e += 1) {
					p_world->storages[i]->notify_changed(entities.entities[e]);
				}
			}
		}
	}
}

void Pipeline::dispatch(World *p_world) {
#ifdef DEBUG_ENABLED
	CRASH_COND_MSG(ready == false, "You can't dispatch a pipeline which is not yet builded. Please call `build`.");
#endif

	p_world->is_dispatching_in_progress = true;

	Hierarchy *hierarchy = static_cast<Hierarchy *>(p_world->get_storage<Child>());
	if (hierarchy) {
		// Flush the hierarchy.
		hierarchy->flush_hierarchy_changes();
	}

	// Process the `TemporarySystem`, if any.
	for (int i = 0; i < int(temporary_systems_exe.size()); i += 1) {
		if (temporary_systems_exe[i](p_world)) {
			temporary_systems_exe.remove(i);
			i -= 1;
		}
	}

	dispatch_sub_dispatcher(p_world, 0);

	// Flush changed.
	for (uint32_t c = 0; c < p_world->storages.size(); c += 1) {
		if (p_world->storages[c] != nullptr) {
			p_world->storages[c]->flush_changed();
		}
	}

	p_world->is_dispatching_in_progress = false;
}

void Pipeline::dispatch_sub_dispatcher(World *p_world, int p_dispatcher_index) {
	ERR_FAIL_INDEX_MSG(p_dispatcher_index, int(dispatchers.size()), "The dispatcher " + itos(p_dispatcher_index) + " doesn't exists in this pipeline. This is a bug, please report it.");

	const DispatcherData &dispatcher = dispatchers[p_dispatcher_index];

	// Dispatch the `Stage`s.
	for (uint32_t stage_i = 0; stage_i < dispatcher.exec_stages.size(); stage_i += 1) {
		// TODO execute in multuple thread.
		for (uint32_t i = 0; i < dispatcher.exec_stages[stage_i].systems.size(); i += 1) {
			dispatcher.exec_stages[stage_i].systems[i].exe(
					p_world,
					this,
					dispatcher.exec_stages[stage_i].systems[i].id);
		}

		// Notify the `System` released the storage for this stage.
		for (uint32_t f = 0; f < dispatcher.exec_stages[stage_i].notify_list_release_write.size(); f += 1) {
			p_world->get_storage(dispatcher.exec_stages[stage_i].notify_list_release_write[f])->on_system_release();
		}
	}

	// Clear any generated component storages.
	for (uint32_t c = 0; c < dispatcher.event_generator.size(); c += 1) {
		p_world->get_storage(dispatcher.event_generator[c])->clear();
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
