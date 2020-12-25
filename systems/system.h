/** @author AndreaCatania */

#pragma once

#include "core/string/string_name.h"
#include "core/templates/local_vector.h"

class World;

// TODO rename using `func_` as prefix.
typedef void (*system_execute)(World *p_world);

// TODO this goes around, which copy the resource a bunch of times.
// Do something about??
struct SystemExeInfo {
	LocalVector<uint32_t> mutable_components;
	LocalVector<uint32_t> immutable_components;
	LocalVector<uint32_t> mutable_resources;
	LocalVector<uint32_t> immutable_resources;
	system_execute system_func = nullptr;
};

// TODO rename using `func_` as prefix.
typedef SystemExeInfo (*get_system_exec_info_func)();
