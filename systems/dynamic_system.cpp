#include "dynamic_system.h"

#include "../pipeline/pipeline.h"
#include "modules/gdscript/gdscript.cpp"

// This include contains the function needed to convert a script system to a
// compile time system.
#include "dynamic_system.gen.h"

godex::DynamicSystemInfo::DynamicSystemInfo() {}

void godex::DynamicSystemInfo::set_system_id(uint32_t p_id) {
	system_id = p_id;
}

void godex::DynamicSystemInfo::set_target(ScriptInstance *p_target) {
	target_script = p_target;
	gdscript_function = nullptr;

	GDScriptInstance *gd_script_instance = dynamic_cast<GDScriptInstance *>(p_target);
	if (gd_script_instance) {
		// This is a GDScript, take the direct function access.
		Ref<GDScript> script = p_target->get_script();
		gdscript_function = script->get_member_functions()[for_each_name];
	}

	target_sub_pipeline = nullptr;
}

void godex::DynamicSystemInfo::with_resource(uint32_t p_resource_id, bool p_mutable) {
	const uint32_t index = resource_element_map.size() + query_element_map.size();
	resource_element_map.push_back(index);
	resources.push_back({ p_resource_id, p_mutable });
}

void godex::DynamicSystemInfo::with_component(uint32_t p_component_id, bool p_mutable) {
	const uint32_t index = resource_element_map.size() + query_element_map.size();
	query_element_map.push_back(index);
	query.with_component(p_component_id, p_mutable);
}

void godex::DynamicSystemInfo::maybe_component(uint32_t p_component_id, bool p_mutable) {
	const uint32_t index = resource_element_map.size() + query_element_map.size();
	query_element_map.push_back(index);
	query.maybe_component(p_component_id, p_mutable);
}

void godex::DynamicSystemInfo::without_component(uint32_t p_component_id) {
	query.without_component(p_component_id);
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

bool godex::DynamicSystemInfo::is_system_dispatcher() const {
	return sub_pipeline_execute != nullptr;
}

EntityID godex::DynamicSystemInfo::get_current_entity_id() const {
	return query.get_current_entity_id();
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

	// Set the resources dependencies.
	for (uint32_t i = 0; i < p_info.resources.size(); i += 1) {
		if (p_info.resources[i].is_mutable) {
			r_out.mutable_resources.push_back(p_info.resources[i].resource_id);
		} else {
			r_out.immutable_resources.push_back(p_info.resources[i].resource_id);
		}
	}

	// Set the components dependencies.
	p_info.query.get_system_info(r_out);

	if (p_info.sub_pipeline_execute) {
		// Extract all the pipeline dependencies.
		p_info.target_sub_pipeline->get_systems_dependencies(r_out);
	}

	r_out.system_func = p_exec;

	// Arrived here, we can assume the system is valid.
	r_out.valid = true;
}

void godex::DynamicSystemInfo::executor(World *p_world, DynamicSystemInfo &p_info) {
	if (p_info.sub_pipeline_execute) {
		// Sub pipeline execution.
		p_info.sub_pipeline_execute(p_world, p_info.target_sub_pipeline);
	} else {
		// Script function.
		ERR_FAIL_COND_MSG(p_info.target_script == nullptr, "[FATAL] This system doesn't have target assigned.");
		ERR_FAIL_COND_MSG(p_info.query.is_valid() == false, "[FATAL] Please check the system " + ECS::get_system_name(p_info.system_id) + " _prepare because the generated query is invalid.");

		// Create the array where the storages are hold.
		LocalVector<AccessResource> resource_access;
		LocalVector<Variant> access;
		LocalVector<Variant *> access_ptr;

		resource_access.reserve(p_info.resources.size());
		access.resize(p_info.resource_element_map.size() + p_info.query_element_map.size());
		access_ptr.resize(access.size());

		// First extract the resources.
		for (uint32_t i = 0; i < p_info.resources.size(); i += 1) {
			// Prepare the `AccessResource`.
			const uint32_t index = resource_access.size();
			resource_access.resize(index + 1);

			resource_access[index].__resource = p_world->get_resource(p_info.resources[i].resource_id);
			resource_access[index].__mut = p_info.resources[i].is_mutable;

			// Assign
			access[p_info.resource_element_map[i]] = &resource_access[index];
			access_ptr[p_info.resource_element_map[i]] = &access[p_info.resource_element_map[i]];
		}

		p_info.query.begin(p_world);
		for (; p_info.query.is_done() == false; p_info.query.next()) {
			// Map the query components, so the function can be called with the
			// right parameter order.
			for (uint32_t c = 0; c < p_info.query.access_count(); c += 1) {
				AccessComponent *ac = p_info.query.get_access(c);
				if (ac->__component) {
					access[p_info.query_element_map[c]] = ac;
				} else {
					// No component, just set null.
					access[p_info.query_element_map[c]] = Variant::NIL;
				}
				access_ptr[p_info.query_element_map[c]] = &access[p_info.query_element_map[c]];
			}

			Callable::CallError err;
			// Call the script function.
			if (p_info.gdscript_function) {
				// Accelerated GDScript function access.
				p_info.gdscript_function->call(
						static_cast<GDScriptInstance *>(p_info.target_script),
						const_cast<const Variant **>(access_ptr.ptr()),
						access_ptr.size(),
						err);
			} else {
				// Other script execution.
				p_info.target_script->call(
						for_each_name,
						const_cast<const Variant **>(access_ptr.ptr()),
						access_ptr.size(),
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
