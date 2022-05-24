#pragma once

#include "core/string/string_name.h"
#include "core/templates/local_vector.h"
#include "core/templates/oa_hash_map.h"
#include "godex/ecs_types.h"

class World;
class Pipeline;

typedef uint64_t (*func_system_data_get_size)();
typedef void (*func_system_data_new_placement)(uint8_t *p_mem, Token p_token, World *p_world, Pipeline *p_pipeline, godex::system_id p_system_id);
typedef void (*func_system_data_delete_placement)(uint8_t *p_mem);
typedef void (*func_system_data_set_active)(uint8_t *p_mem, bool p_active);

typedef bool (*func_temporary_system_execute)(uint8_t *p_mem, World *p_world);
typedef void (*func_system_execute)(uint8_t *p_mem, World *p_world);
typedef uint32_t (*func_system_dispatcher_execute)(uint8_t *p_mem, World *p_world);

struct SystemExeInfo {
	bool valid = true;
	RBSet<uint32_t> mutable_components;
	RBSet<uint32_t> immutable_components;
	RBSet<uint32_t> mutable_components_storage;
	RBSet<uint32_t> mutable_databags;
	RBSet<uint32_t> immutable_databags;
	RBSet<uint32_t> events_emitters;
	OAHashMap<uint32_t, RBSet<String>> events_receivers;

	// Used if the system is a normal system.
	func_system_execute system_func = nullptr;

	void clear() {
		valid = true;

		mutable_components.clear();
		immutable_components.clear();
		mutable_components_storage.clear();
		mutable_databags.clear();
		immutable_databags.clear();
		events_emitters.clear();
		events_receivers.clear();

		system_func = nullptr;
	}
};

typedef void (*func_get_system_exe_info)(godex::system_id, SystemExeInfo &);
