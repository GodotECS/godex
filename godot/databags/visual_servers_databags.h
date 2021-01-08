#pragma once

#include "../../databags/databag.h"

#include "servers/rendering_server.h"

class RenderingServerDatabag : public godex::Databag {
	DATABAG(RenderingServerDatabag)

	RenderingServer *rs = nullptr;

public:
	RenderingServerDatabag();

	const RenderingServer *get_rs() const;
	RenderingServer *get_rs();
};

class RenderingScenarioDatabag : public godex::Databag {
	DATABAG(RenderingScenarioDatabag)

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
