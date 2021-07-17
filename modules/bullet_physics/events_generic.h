#pragma once

#include "../../events/events.h"

struct OverlapStart {
	EVENT(OverlapStart)

	EntityID area;
	EntityID other_body;

	static void _bind_method();
};

struct OverlapEnd {
	EVENT(OverlapEnd)

	EntityID area;
	EntityID other_body;

	static void _bind_method();
};
