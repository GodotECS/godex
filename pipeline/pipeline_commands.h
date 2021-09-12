#pragma once

#include "../databags/databag.h"

struct WorldData;
class Pipeline;

class PipelineCommands : public godex::Databag {
	DATABAG(PipelineCommands)

	friend class Pipeline;

	static void _bind_methods();

	WorldData *world_data = nullptr;
	Pipeline *pipeline = nullptr;

public:
	void set_active_system(const StringName &p_system_name, bool p_active);
};
