#include "dynamic_system.h"

#include "../pipeline/pipeline.h"
#include "../utils/fetchers.h"
#include "modules/gdscript/gdscript.h"

godex::DynamicSystemExecutionData::DynamicSystemExecutionData() {}

godex::DynamicSystemExecutionData::~DynamicSystemExecutionData() {
	reset();
}

void godex::DynamicSystemExecutionData::set_system_id(uint32_t p_id) {
	system_id = p_id;
}

void godex::DynamicSystemExecutionData::set_target(ScriptInstance *p_target) {
	target_script = p_target;
	gdscript_function = nullptr;
}

void godex::DynamicSystemExecutionData::execute_in(Phase p_phase, const StringName &p_dispatcher_name) {
	ECS::get_system_info(system_id).execute_in(p_phase, p_dispatcher_name);
}

void godex::DynamicSystemExecutionData::execute_after(const StringName &p_system_name) {
	ECS::get_system_info(system_id).after(p_system_name);
}

void godex::DynamicSystemExecutionData::execute_before(const StringName &p_system_name) {
	ECS::get_system_info(system_id).before(p_system_name);
}

void godex::DynamicSystemExecutionData::with_query(DynamicQuery *p_query) {
	CRASH_COND_MSG(compiled, "The system is already build, this function can't be called now.");
	fetchers.push_back(p_query);
}

void godex::DynamicSystemExecutionData::with_databag(uint32_t p_databag_id, bool p_mutable) {
	CRASH_COND_MSG(compiled, "The system is already build, this function can't be called now.");
	DatabagDynamicFetcher *fetcher = memnew(DatabagDynamicFetcher);
	fetcher->init(p_databag_id, p_mutable);
	fetchers.push_back(fetcher);
}

void godex::DynamicSystemExecutionData::with_storage(godex::component_id p_component_id) {
	CRASH_COND_MSG(compiled, "The system is already build, this function can't be called now.");

	StorageDynamicFetcher *fetcher = memnew(StorageDynamicFetcher);
	fetcher->init(p_component_id);
	fetchers.push_back(fetcher);
}

void godex::DynamicSystemExecutionData::with_events_emitter(godex::event_id p_event_id) {
	CRASH_COND_MSG(compiled, "This function can be called only within the prepare function.");
	EventsEmitterDynamicFetcher *fetcher = memnew(EventsEmitterDynamicFetcher);
	fetcher->init(p_event_id);
	fetchers.push_back(fetcher);
}

void godex::DynamicSystemExecutionData::with_events_receiver(godex::event_id p_event_id, const String &p_emitter_name) {
	CRASH_COND_MSG(compiled, "This function can be called only within the prepare function.");
	EventsReceiverDynamicFetcher *fetcher = memnew(EventsReceiverDynamicFetcher);
	fetcher->init(p_event_id, p_emitter_name);
	fetchers.push_back(fetcher);
}

void godex::DynamicSystemExecutionData::prepare_world(World *p_world) {
	CRASH_COND_MSG(compiled, "The DynamicSystem is not supposed to be compiled twice.");
	compiled = true;
	world = p_world;

	// ~~ If the script is a GDScript instance, take the function pointer. ~~
	GDScriptInstance *gd_script_instance = dynamic_cast<GDScriptInstance *>(target_script);
	if (gd_script_instance) {
		// This is a GDScript, take the direct function access.
		Ref<GDScript> script = target_script->get_script();
		gdscript_function = script->get_member_functions()[SNAME("_execute")];
	}

	access.resize(fetchers.size());
	access_ptr.resize(access.size());

	for (uint32_t i = 0; i < fetchers.size(); i += 1) {
		fetchers[i]->prepare_world(p_world);
		access[i] = fetchers[i];
		access_ptr[i] = &access[i];
	}
}

void godex::DynamicSystemExecutionData::set_active(bool p_active) {
	for (uint32_t i = 0; i < fetchers.size(); i += 1) {
		fetchers[i]->set_active(p_active);
	}
}

void godex::DynamicSystemExecutionData::reset() {
	for (uint32_t i = 0; i < fetchers.size(); i += 1) {
		fetchers[i]->release_world(world);
	}

	target_script = nullptr;
	compiled = false;
	gdscript_function = nullptr;
	system_id = UINT32_MAX;
	world = nullptr;

	access.reset();
	access_ptr.reset();

	for (uint32_t i = 0; i < fetchers.size(); i += 1) {
		memdelete(fetchers[i]);
	}
	fetchers.reset();
}

void godex::DynamicSystemExecutionData::get_info(DynamicSystemExecutionData &p_info, SystemExeInfo &r_out) {
	for (uint32_t i = 0; i < p_info.fetchers.size(); i += 1) {
		p_info.fetchers[i]->get_system_info(&r_out);
	}

	r_out.system_func = godex::DynamicSystemExecutionData::executor;

	// Arrived here, we can assume the system is valid.
	r_out.valid = true;
}

void godex::DynamicSystemExecutionData::executor(uint8_t *p_mem, World *p_world) {
	godex::DynamicSystemExecutionData *p_info = (godex::DynamicSystemExecutionData *)p_mem;
	ERR_FAIL_COND_MSG(p_info->compiled == false, "The System is not supposed to be executed without being compiled. Maybe this System is invalid? System: " + ECS::get_system_name(p_info->system_id));

	// Script function.
	ERR_FAIL_COND_MSG(p_info->target_script == nullptr, "[FATAL] This system doesn't have target assigned.");

	for (uint32_t i = 0; i < p_info->fetchers.size(); i += 1) {
		p_info->fetchers[i]->initiate_process(p_world);
	}

	Callable::CallError err;
	// Call the script function.
	if (p_info->gdscript_function) {
		// Accelerated GDScript function access.
		p_info->gdscript_function->call(
				static_cast<GDScriptInstance *>(p_info->target_script),
				const_cast<const Variant **>(p_info->access_ptr.ptr()),
				p_info->access_ptr.size(),
				err);
	} else {
		// Other script execution.
		p_info->target_script->callp(
				SNAME("_execute"),
				const_cast<const Variant **>(p_info->access_ptr.ptr()),
				p_info->access_ptr.size(),
				err);
	}

	for (uint32_t i = 0; i < p_info->fetchers.size(); i += 1) {
		p_info->fetchers[i]->conclude_process(p_world);
	}

	ERR_FAIL_COND_MSG(err.error != Callable::CallError::CALL_OK, "System function execution error: " + itos(err.error) + " System name: " + ECS::get_system_name(p_info->system_id) + ". Please check the parameters.");
}
