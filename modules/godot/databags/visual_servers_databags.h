#pragma once

#include "../../../databags/databag.h"

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
	static void _bind_methods();

	// TODO Probably this should be an array to support more scenarios.
	RID scenario;
};
