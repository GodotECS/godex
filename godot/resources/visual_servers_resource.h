#pragma once

#include "../../resources/ecs_resource.h"

#include "servers/rendering_server.h"

class RenderingServerResource : public godex::Resource {
	RESOURCE(RenderingServerResource)

	RenderingServer *rs = nullptr;

public:
	RenderingServerResource();

	const RenderingServer *get_rs() const;
	RenderingServer *get_rs();
};

class RenderingScenarioResource : public godex::Resource {
	RESOURCE(RenderingScenarioResource)

	// TODO Probably this should be an array to support more scenarios.
	RID scenario;
	bool changed = false;

	static void _bind_properties();

public:
	void set_scenario(RID p_scenario_rid);
	RID get_scenario() const;

	void set_changed(bool p_changed);
	bool is_changed() const;
};
