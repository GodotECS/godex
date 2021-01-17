#include "pipeline.h"

#include "../ecs.h"
#include "../world/world.h"

Pipeline::Pipeline() {
}

// Unset the macro defined into the `pipeline.h` so to properly point the method
// definition.
#undef add_system
uint32_t Pipeline::add_system(func_get_system_exe_info p_func_get_exe_info) {
#ifdef DEBUG_ENABLED
	// This is automated by the `add_system` macro or by
	// `ECS::register_system` macro, so is never supposed to happen.
	CRASH_COND_MSG(p_func_get_exe_info == nullptr, "The passed system constructor can't be nullptr at this point.");
	// Using crash cond because pipeline composition is not directly exposed
	// to the user.
	CRASH_COND_MSG(is_ready(), "The pipeline is ready, you can't modify it.");
#endif
	const uint32_t in_pipeline_id = systems_info.size();
	systems_info.push_back(p_func_get_exe_info);
	return in_pipeline_id;
}

uint32_t Pipeline::add_registered_system(godex::system_id p_id) {
	const uint32_t in_pipeline_id = add_system(ECS::get_func_system_exe_info(p_id));
	if (ECS::is_system_dispatcher(p_id)) {
		system_dispatchers.push_back(in_pipeline_id);
	}
	return in_pipeline_id;
}

void Pipeline::build() {
#ifdef DEBUG_ENABLED
	CRASH_COND_MSG(ready, "You can't build a pipeline twice.");
#endif
	ready = true;

	systems_exe.reserve(systems_info.size());

	SystemExeInfo info;
	for (uint32_t i = 0; i < systems_info.size(); i += 1) {
		info.clear();
		systems_info[i](info);

		ERR_CONTINUE_MSG(info.valid == false, "[FATAL][FATAL][FATAL][PIPELINE-FATAL] The system with index: " + itos(i) + " is invalid. Excluded from pipeline.");

#ifdef DEBUG_ENABLED
		// This is automated by the `add_system` macro or by
		// `ECS::register_system` macro, so is never supposed to happen.
		CRASH_COND_MSG(info.system_func == nullptr, "At this point `info.system_func` is supposed to be not null. To add a system use the following syntax: `add_system(function_name);` or use the `ECS` class to get the `SystemExeInfo` if it's a registered system.");
#endif

		const bool is_system_dispatcher = system_dispatchers.find(i) != -1;

		for (uint32_t c = 0; c < info.mutable_components_storage.size(); c += 1) {
			// Take the events that are generated by this pipeline
			// (no sub pipelines).
			if (is_system_dispatcher == false) {
				if (ECS::is_component_events(info.mutable_components_storage[c])) {
					// Make sure it's unique
					if (event_generator.find(info.mutable_components_storage[c]) == -1) {
						event_generator.push_back(info.mutable_components_storage[c]);
					}
				}
			}

			// Take the components created by the systems of this pipeline.
			if (component_generator.find(info.mutable_components_storage[c]) == -1) {
				component_generator.push_back(info.mutable_components_storage[c]);
			}
		}

		systems_exe.push_back(info.system_func);
	}
}

bool Pipeline::is_ready() const {
	return ready;
}

void Pipeline::get_systems_dependencies(SystemExeInfo &p_info) const {
	SystemExeInfo other_info;
	for (uint32_t i = 0; i < systems_info.size(); i += 1) {
		other_info.clear();

		systems_info[i](other_info);

		// Handles the Components.
		for (uint32_t t = 0; t < other_info.immutable_components.size(); t += 1) {
			const godex::component_id id = other_info.immutable_components[t];

			const bool is_unique = p_info.immutable_components.find(id) == -1;
			if (is_unique) {
				p_info.immutable_components.push_back(id);
			}
		}

		for (uint32_t t = 0; t < other_info.mutable_components.size(); t += 1) {
			const godex::component_id id = other_info.mutable_components[t];

			const bool is_unique = p_info.mutable_components.find(id) == -1;
			if (is_unique) {
				p_info.mutable_components.push_back(id);
			}
		}

		// Handles the Databags.
		for (uint32_t t = 0; t < other_info.immutable_databags.size(); t += 1) {
			const godex::databag_id id = other_info.immutable_databags[t];

			const bool is_unique = p_info.immutable_databags.find(id) == -1;
			if (is_unique) {
				p_info.immutable_databags.push_back(id);
			}
		}

		for (uint32_t t = 0; t < other_info.mutable_databags.size(); t += 1) {
			const godex::databag_id id = other_info.mutable_databags[t];

			const bool is_unique = p_info.mutable_databags.find(id) == -1;
			if (is_unique) {
				p_info.mutable_databags.push_back(id);
			}
		}
	}
}

void Pipeline::reset() {
	systems_info.clear();
	systems_exe.clear();
	ready = false;
}

void Pipeline::prepare(World *p_world) {
	// Make sure the storages exists at this point.
	for (uint32_t c = 0; c < component_generator.size(); c += 1) {
		p_world->create_storage(component_generator[c]);
	}
}

void Pipeline::dispatch(World *p_world) {
#ifdef DEBUG_ENABLED
	CRASH_COND_MSG(ready == false, "You can't dispatch a pipeline which is not yet builded. Please call `build`.");
#endif

	for (uint32_t i = 0; i < systems_exe.size(); i += 1) {
		systems_exe[i](p_world);
	}

	// Clear any generated component storages.
	for (uint32_t c = 0; c < event_generator.size(); c += 1) {
		p_world->get_storage(event_generator[c])->clear();
	}
}
