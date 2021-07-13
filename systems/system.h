#pragma once

#include "core/string/string_name.h"
#include "core/templates/local_vector.h"
#include "core/templates/oa_hash_map.h"

class World;
class Pipeline;

typedef bool (*func_temporary_system_execute)(World *p_world);
typedef void (*func_system_execute)(World *p_world, Pipeline *p_pipeline, uint32_t p_system_id);
typedef uint32_t (*func_system_dispatcher_execute)(World *p_world);

struct SystemExeInfo {
	bool valid = true;
	Set<uint32_t> mutable_components;
	Set<uint32_t> immutable_components;
	Set<uint32_t> mutable_components_storage;
	Set<uint32_t> mutable_databags;
	Set<uint32_t> immutable_databags;
	Set<uint32_t> need_changed;
	Set<uint32_t> events_emitters;
	OAHashMap<uint32_t, Set<String>> events_receivers;
	// Used if the system is a normal system.
	func_system_execute system_func = nullptr;

	void clear() {
		valid = true;
		mutable_components.clear();
		immutable_components.clear();
		mutable_components_storage.clear();
		mutable_databags.clear();
		immutable_databags.clear();
		need_changed.clear();
		system_func = nullptr;
	}
};

typedef void (*func_get_system_exe_info)(SystemExeInfo &);
