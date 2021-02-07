#include "dynamic_system.h"

#include "../pipeline/pipeline.h"
#include "modules/gdscript/gdscript.cpp"

// This include contains the function needed to convert a script system to a
// compile time system.
#include "dynamic_system.gen.h"

void godex::__dynamic_system_info_static_destructor() {
	for (uint32_t i = 0; i < DYNAMIC_SYSTEMS_MAX; i += 1) {
		dynamic_info[i].reset();
	}
	registered_dynamic_system_count = 0;
}

godex::DynamicSystemInfo::DynamicSystemInfo() {}

void godex::DynamicSystemInfo::set_system_id(uint32_t p_id) {
	system_id = p_id;
}

void godex::DynamicSystemInfo::set_target(ScriptInstance *p_target) {
	target_script = p_target;
	gdscript_function = nullptr;
	target_sub_pipeline = nullptr;
}

void godex::DynamicSystemInfo::set_space(Space p_space) {
	CRASH_COND_MSG(compiled, "The query can't be composed, when the system is already been compiled.");

	query.set_space(p_space);
}

void godex::DynamicSystemInfo::with_databag(uint32_t p_databag_id, bool p_mutable) {
	CRASH_COND_MSG(compiled, "The query can't be composed, when the system is already been compiled.");

	const uint32_t index = databag_element_map.size() + storage_element_map.size() + query_element_map.size();
	databag_element_map.push_back(index);
	databags.push_back({ p_databag_id, p_mutable });
}

void godex::DynamicSystemInfo::with_component(uint32_t p_component_id, bool p_mutable) {
	CRASH_COND_MSG(compiled, "The query can't be composed, when the system is already been compiled.");

	const uint32_t index = databag_element_map.size() + storage_element_map.size() + query_element_map.size();
	query_element_map.push_back(index);
	query.with_component(p_component_id, p_mutable);
}

void godex::DynamicSystemInfo::maybe_component(uint32_t p_component_id, bool p_mutable) {
	CRASH_COND_MSG(compiled, "The query can't be composed, when the system is already been compiled.");

	const uint32_t index = databag_element_map.size() + storage_element_map.size() + query_element_map.size();
	query_element_map.push_back(index);
	query.maybe_component(p_component_id, p_mutable);
}

void godex::DynamicSystemInfo::without_component(uint32_t p_component_id) {
	CRASH_COND_MSG(compiled, "The query can't be composed, when the system is already been compiled.");

	query.without_component(p_component_id);
}

void godex::DynamicSystemInfo::with_storage(uint32_t p_component_id) {
	CRASH_COND_MSG(compiled, "The query can't be composed, when the system is already been compiled.");

	const uint32_t index = databag_element_map.size() + storage_element_map.size() + query_element_map.size();
	storage_element_map.push_back(index);
	storages.push_back(p_component_id);
}

void godex::DynamicSystemInfo::set_target(func_system_execute_pipeline p_system_exe) {
	target_script = nullptr;
	gdscript_function = nullptr;
	sub_pipeline_execute = p_system_exe;
}

void godex::DynamicSystemInfo::set_pipeline(Pipeline *p_target) {
	CRASH_COND_MSG(sub_pipeline_execute == nullptr, "The pipeline execute target can't be nullptr at this point. Call `set_target()` before this function.");
	target_script = nullptr;
	target_sub_pipeline = p_target;
}

bool godex::DynamicSystemInfo::build() {
	CRASH_COND_MSG(compiled, "The query is not supposed to be compiled twice.");
	compiled = true;

	// ~~ If the script is a GDScript instance, take the function pointer. ~~
	GDScriptInstance *gd_script_instance = dynamic_cast<GDScriptInstance *>(target_script);
	if (gd_script_instance) {
		// This is a GDScript, take the direct function access.
		Ref<GDScript> script = target_script->get_script();
		gdscript_function = script->get_member_functions()[for_each_name];
	}

	query.build();

	// ~~ Init the script accessors. ~~
	{
		access.resize(databag_element_map.size() + storage_element_map.size() + query_element_map.size());
		access_ptr.resize(access.size());

		// ~~ Databags
		databag_accessors.resize(databags.size());

		// Set the databag accessors.
		for (uint32_t i = 0; i < databags.size(); i += 1) {
			// Set the mutability.
			// Creating a new pointer because `set_script_instance` handles
			// the pointer lifetime unfortunately so set an automatic memory
			// pointer is not safe.
			databag_accessors[i].__mut = databags[i].is_mutable;

			// Assign the accessor.
			access[databag_element_map[i]] = &databag_accessors[i];
			access_ptr[databag_element_map[i]] = &access[databag_element_map[i]];
		}

		// ~~ Storages
		storage_accessors.resize(storages.size());

		// Set the storage accessors.
		for (uint32_t i = 0; i < storages.size(); i += 1) {
			// The storages are always mutable.
			storage_accessors[i].__mut = true;

			// Assign the accessor.
			access[storage_element_map[i]] = &storage_accessors[i];
			access_ptr[storage_element_map[i]] = &access[storage_element_map[i]];
		}

		// Init the query accessors.
		// Map the query components, so the function can be called with the
		// right parameter order.
		// It's fine store the accessor pointers here because the query is
		// stored together with the `DynamicSystemInfo`.
		for (uint32_t c = 0; c < query.access_count(); c += 1) {
			Object *ac = query.get_access(c);
			access[query_element_map[c]] = ac;
			access_ptr[query_element_map[c]] = &access[query_element_map[c]];
		}
	}

	return true;
}

bool godex::DynamicSystemInfo::is_system_dispatcher() const {
	return sub_pipeline_execute != nullptr;
}

EntityID godex::DynamicSystemInfo::get_current_entity_id() const {
	return query.get_current_entity_id();
}

void godex::DynamicSystemInfo::reset() {
	target_script = nullptr;
	compiled = false;
	gdscript_function = nullptr;
	system_id = UINT32_MAX;
	databag_element_map.reset();
	storage_element_map.reset();
	query_element_map.reset();
	databags.reset();
	storages.reset();
	query.reset();
	access.reset();
	access_ptr.reset();
	databag_accessors.reset();
	storage_accessors.reset();
	sub_pipeline_execute = nullptr;
	target_sub_pipeline = nullptr;
}

StringName godex::DynamicSystemInfo::for_each_name;

void godex::DynamicSystemInfo::get_info(DynamicSystemInfo &p_info, func_system_execute p_exec, SystemExeInfo &r_out) {
	// Assume is invalid.
	r_out.valid = false;

	// Validate.
	if (p_info.sub_pipeline_execute) {
		ERR_FAIL_COND_MSG(p_info.target_sub_pipeline == nullptr, "No sub pipeline set.");

#ifdef DEBUG_ENABLED
		// The pipeline must be fully build at this point
		CRASH_COND_MSG(p_info.target_sub_pipeline->is_ready() == false, "The sub pipeline is not yet builded. Make sure to fully build it before using it as sub pipeline.");
#endif

	} else {
		// Script function.
		ERR_FAIL_COND_MSG(p_info.target_script == nullptr, "[FATAL] This system doesn't have target assigned.");

		// Script execution.
		ERR_FAIL_COND(p_info.query.is_valid() == false);
	}

	// Set the components dependencies.
	p_info.query.get_system_info(r_out);

	// Set the storages dependencies.
	for (uint32_t i = 0; i < p_info.storages.size(); i += 1) {
		r_out.mutable_components_storage.push_back(p_info.storages[i]);
	}

	// Set the databags dependencies.
	for (uint32_t i = 0; i < p_info.databags.size(); i += 1) {
		if (p_info.databags[i].is_mutable) {
			r_out.mutable_databags.push_back(p_info.databags[i].databag_id);
		} else {
			r_out.immutable_databags.push_back(p_info.databags[i].databag_id);
		}
	}

	if (p_info.sub_pipeline_execute) {
		// Extract all the pipeline dependencies.
		p_info.target_sub_pipeline->get_systems_dependencies(r_out);
	}

	r_out.system_func = p_exec;

	// Arrived here, we can assume the system is valid.
	r_out.valid = true;
}

void godex::DynamicSystemInfo::executor(World *p_world, DynamicSystemInfo &p_info) {
	CRASH_COND_MSG(p_info.compiled == false, "The query is not supposed to be executed without being compiled.");

	if (p_info.sub_pipeline_execute) {
		// Sub pipeline execution.
		p_info.sub_pipeline_execute(p_world, p_info.target_sub_pipeline);
	} else {
		// Script function.
		ERR_FAIL_COND_MSG(p_info.target_script == nullptr, "[FATAL] This system doesn't have target assigned.");
		ERR_FAIL_COND_MSG(p_info.query.is_valid() == false, "[FATAL] Please check the system " + ECS::get_system_name(p_info.system_id) + " _prepare because the generated query is invalid.");

		// First extract the databags.
		for (uint32_t i = 0; i < p_info.databags.size(); i += 1) {
			// Set the accessors pointers.
			p_info.databag_accessors[i].__target = p_world->get_databag(p_info.databags[i].databag_id);
		}

		// Then extract the storages.
		for (uint32_t i = 0; i < p_info.storages.size(); i += 1) {
			// Set the accessors pointers.
			p_info.storage_accessors[i].__target = p_world->get_storage(p_info.storages[i]);
		}

		// Execute the query

		p_info.query.begin(p_world);
		for (; p_info.query.is_done() == false; p_info.query.next()) {
			Callable::CallError err;
			// Call the script function.
			if (p_info.gdscript_function) {
				// Accelerated GDScript function access.
				p_info.gdscript_function->call(
						static_cast<GDScriptInstance *>(p_info.target_script),
						const_cast<const Variant **>(p_info.access_ptr.ptr()),
						p_info.access_ptr.size(),
						err);
			} else {
				// Other script execution.
				p_info.target_script->call(
						for_each_name,
						const_cast<const Variant **>(p_info.access_ptr.ptr()),
						p_info.access_ptr.size(),
						err);
			}
			if (err.error != Callable::CallError::CALL_OK) {
				p_info.query.end();
				ERR_FAIL_COND_MSG(err.error != Callable::CallError::CALL_OK, "System function execution error: " + itos(err.error) + " System name: " + ECS::get_system_name(p_info.system_id) + ". Please check the parameters.");
			}
		}
		p_info.query.end();
	}
}
