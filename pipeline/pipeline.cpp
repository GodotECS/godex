#include "pipeline.h"

#include "../world/world.h"

Pipeline::Pipeline() {
}

void Pipeline::add_registered_system(const SystemExeInfo &p_system_info) {
	CRASH_COND_MSG(p_system_info.system_func == nullptr, "At this point `info.system_func` is supposed to be not null. To add a system use the following syntax: `add_system(function_name);` or use the `ECS` class to get the `SystemExeInfo` if it's a registered system.");
	//print_line(
	//		"Added function that has " + itos(info.mutable_components.size()) +
	//		" mut comp, " + itos(info.immutable_components.size()) + " immutable comp");

	// TODO compose the pipeline
	systems.push_back(p_system_info.system_func);
}

// Unset the macro defined into the `pipeline.h` so to properly point the method
// definition.
#undef add_system
void Pipeline::add_system(get_system_exec_info_func p_get_info_func) {
	const SystemExeInfo info = p_get_info_func();
	add_registered_system(info);
}

void Pipeline::dispatch(World *p_world) {
	for (uint32_t i = 0; i < systems.size(); i += 1) {
		systems[i](p_world);
	}
}
