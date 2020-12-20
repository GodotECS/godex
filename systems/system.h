/** @author AndreaCatania */

#pragma once

#include "core/string/string_name.h"
#include "core/templates/local_vector.h"

class World;

typedef void (*system_execute)(World *p_world);

// TODO this goes around, which copy the resource a bunch of times.
// Do something about??
struct SystemInfo {
	// TODO do I really need the name here?
	StringName name;
	String description;
	LocalVector<uint32_t> mutable_components;
	LocalVector<uint32_t> immutable_components;
	LocalVector<uint32_t> mutable_resources;
	LocalVector<uint32_t> immutable_resources;
	system_execute system_func = nullptr;
};

typedef SystemInfo (*get_system_info_func)();
