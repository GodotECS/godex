#pragma once

#include "core/string/string_name.h"
#include "core/templates/local_vector.h"

enum Phase : int {
	PHASE_CONFIG = 0,
	PHASE_INPUT,
	PHASE_PRE_PROCESS,
	PHASE_PROCESS,
	PHASE_POST_PROCESS,
	PHASE_MAX,
};

struct Dependency {
	bool execute_before;
	StringName system_name;
};

struct SystmeDescriptor {
	Phase phase;
	StringName name;
	LocalVector<Dependency> dependencies;
	LocalVector<StringName> component_write;
	LocalVector<StringName> component_read;
	LocalVector<StringName> databag_write;
	LocalVector<StringName> databag_read;
};
