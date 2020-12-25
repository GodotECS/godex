#include "pipeline.h"

#include "../ecs.h"
#include "../world/world.h"

Pipeline::Pipeline() {
}

// Unset the macro defined into the `pipeline.h` so to properly point the method
// definition.
#undef add_system
void Pipeline::add_system(get_system_exec_info_func p_get_info_func) {
#ifdef DEBUG_ENABLED
	// This is automated by the `add_system` macro or by
	// `ECS::register_system` macro, so is never supposed to happen.
	CRASH_COND_MSG(p_get_info_func == nullptr, "The passed system constructor can't be nullptr at this point.");
	// Using crash cond because pipeline composition is not directly exposed
	// to the user.
	CRASH_COND_MSG(is_ready(), "The pipeline is ready, you can't modify it.");
#endif
	systems_info.push_back(p_get_info_func);
}

void Pipeline::add_registered_system(godex::system_id p_id) {
	add_system(ECS::get_func_system_exe_info(p_id));
}

void Pipeline::build() {
#ifdef DEBUG_ENABLED
	CRASH_COND_MSG(ready, "You can't build a pipeline twice.");
#endif
	ready = true;

	systems.reserve(systems_info.size());

	for (uint32_t i = 0; i < systems_info.size(); i += 1) {
		const SystemExeInfo info = systems_info[i]();
#ifdef DEBUG_ENABLED
		// This is automated by the `add_system` macro or by
		// `ECS::register_system` macro, so is never supposed to happen.
		CRASH_COND_MSG(info.system_func == nullptr, "At this point `info.system_func` is supposed to be not null. To add a system use the following syntax: `add_system(function_name);` or use the `ECS` class to get the `SystemExeInfo` if it's a registered system.");
#endif
		systems.push_back(info.system_func);
	}
}

bool Pipeline::is_ready() const {
	return ready;
}

void Pipeline::get_systems_dependencies(SystemExeInfo &p_info) const {
	for (uint32_t i = 0; i < systems_info.size(); i += 1) {
		const SystemExeInfo other_info = systems_info[i]();

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

		// Handles the Resources.
		for (uint32_t t = 0; t < other_info.immutable_resources.size(); t += 1) {
			const godex::resource_id id = other_info.immutable_resources[t];

			const bool is_unique = p_info.immutable_resources.find(id) == -1;
			if (is_unique) {
				p_info.immutable_resources.push_back(id);
			}
		}

		for (uint32_t t = 0; t < other_info.mutable_resources.size(); t += 1) {
			const godex::resource_id id = other_info.mutable_resources[t];

			const bool is_unique = p_info.mutable_resources.find(id) == -1;
			if (is_unique) {
				p_info.mutable_resources.push_back(id);
			}
		}
	}
}

void Pipeline::reset() {
	systems_info.clear();
	systems.clear();
	ready = false;
}

void Pipeline::dispatch(World *p_world) {
#ifdef DEBUG_ENABLED
	CRASH_COND_MSG(ready == false, "You can't dispatch a pipeline which is not yet builded. Please call `build`.");
#endif
	for (uint32_t i = 0; i < systems.size(); i += 1) {
		systems[i](p_world);
	}
}
