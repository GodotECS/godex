/** @author AndreaCatania */

#pragma once

#include "core/string/string_name.h"
#include "core/templates/local_vector.h"

class World;

typedef void (*func_system_execute)(World *p_world);

struct SystemExeInfo {
	LocalVector<uint32_t> mutable_components;
	LocalVector<uint32_t> immutable_components;
	LocalVector<uint32_t> mutable_resources;
	LocalVector<uint32_t> immutable_resources;
	func_system_execute system_func = nullptr;

	void clear() {
		mutable_components.clear();
		immutable_components.clear();
		mutable_resources.clear();
		immutable_resources.clear();
		system_func = nullptr;
	}
};

typedef void (*func_get_system_exe_info)(SystemExeInfo &);
