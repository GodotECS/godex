#include "pipeline_commands.h"

#include "pipeline.h"

void PipelineCommands::_bind_methods() {
	add_method("set_active_system", &PipelineCommands::set_active_system);
}

void PipelineCommands::set_active_system(const StringName &p_system_name, bool p_active) {
	ERR_FAIL_COND_MSG(pipeline == nullptr, "The pipeline is not set, maybe it's not processing?");
	const godex::system_id system_id = ECS::get_system_id(p_system_name);
	ERR_FAIL_COND_MSG(system_id == godex::SYSTEM_NONE, "The system `" + p_system_name + "` doesn't exist.");
	for (uint32_t d_i = 0; d_i < pipeline->dispatchers.size(); d_i += 1) {
		for (uint32_t s_i = 0; s_i < pipeline->dispatchers[d_i].exec_stages.size(); s_i += 1) {
			for (uint32_t sys_i = 0; sys_i < pipeline->dispatchers[d_i].exec_stages[s_i].systems.size(); sys_i += 1) {
				ExecutionSystemData *system_data = pipeline->dispatchers[d_i].exec_stages[s_i].systems.ptr() + sys_i;
				if (system_data->id == system_id) {
					// System found!
					ECS::system_set_active_system(
							system_id,
							world_data->system_data[system_data->index],
							p_active);

					// Notice, here there isn't a break so we can make sure all the
					// system instances are disabled.
				}
			}
		}
	}
}
