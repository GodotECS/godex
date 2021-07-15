#include "dynamic_system.h"

#include "../pipeline/pipeline.h"
#include "../utils/fetchers.h"
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

godex::DynamicSystemInfo::~DynamicSystemInfo() {
	reset();
}

void godex::DynamicSystemInfo::set_system_id(uint32_t p_id) {
	system_id = p_id;
}

void godex::DynamicSystemInfo::set_target(ScriptInstance *p_target) {
	target_script = p_target;
	gdscript_function = nullptr;
}

void godex::DynamicSystemInfo::execute_in(Phase p_phase, const StringName &p_dispatcher_name) {
	ECS::get_system_info(system_id).execute_in(p_phase, p_dispatcher_name);
}

void godex::DynamicSystemInfo::execute_after(const StringName &p_system_name) {
	ECS::get_system_info(system_id).after(p_system_name);
}

void godex::DynamicSystemInfo::execute_before(const StringName &p_system_name) {
	ECS::get_system_info(system_id).before(p_system_name);
}

void godex::DynamicSystemInfo::with_query(DynamicQuery *p_query) {
	CRASH_COND_MSG(compiled, "The system is already build, this function can't be called now.");
	p_query->build();
	fetchers.push_back(p_query);
}

void godex::DynamicSystemInfo::with_databag(uint32_t p_databag_id, bool p_mutable) {
	CRASH_COND_MSG(compiled, "The system is already build, this function can't be called now.");
	DatabagDynamicFetcher *fetcher = memnew(DatabagDynamicFetcher);
	fetcher->init(p_databag_id, p_mutable);
	fetchers.push_back(fetcher);
}

void godex::DynamicSystemInfo::with_storage(godex::component_id p_component_id) {
	CRASH_COND_MSG(compiled, "The system is already build, this function can't be called now.");

	StorageDynamicFetcher *fetcher = memnew(StorageDynamicFetcher);
	fetcher->init(p_component_id);
	fetchers.push_back(fetcher);
}

void godex::DynamicSystemInfo::with_event_emitter(godex::event_id p_event_id) {
	CRASH_COND_MSG(compiled, "This function can be called only within the prepare function.");
	EventsEmitterDynamicFetcher *fetcher = memnew(EventsEmitterDynamicFetcher);
	fetcher->init(p_event_id);
	fetchers.push_back(fetcher);
}

void godex::DynamicSystemInfo::with_event_receiver(godex::event_id p_event_id, const String &p_emitter_name) {
	CRASH_COND_MSG(compiled, "This function can be called only within the prepare function.");
	EventsReceiverDynamicFetcher *fetcher = memnew(EventsReceiverDynamicFetcher);
	fetcher->init(p_event_id, p_emitter_name);
	fetchers.push_back(fetcher);
}

bool godex::DynamicSystemInfo::build() {
	CRASH_COND_MSG(compiled, "The query is not supposed to be compiled twice.");
	compiled = true;

	// ~~ If the script is a GDScript instance, take the function pointer. ~~
	GDScriptInstance *gd_script_instance = dynamic_cast<GDScriptInstance *>(target_script);
	if (gd_script_instance) {
		// This is a GDScript, take the direct function access.
		Ref<GDScript> script = target_script->get_script();
		gdscript_function = script->get_member_functions()[execute_func_name];
	}

	access.resize(fetchers.size());
	access_ptr.resize(access.size());

	for (uint32_t i = 0; i < fetchers.size(); i += 1) {
		access[i] = fetchers[i];
		access_ptr[i] = &access[i];
	}

	return true;
}

void godex::DynamicSystemInfo::reset() {
	target_script = nullptr;
	compiled = false;
	gdscript_function = nullptr;
	system_id = UINT32_MAX;

	access.reset();
	access_ptr.reset();

	for (uint32_t i = 0; i < fetchers.size(); i += 1) {
		memdelete(fetchers[i]);
	}
	fetchers.reset();
}

StringName godex::DynamicSystemInfo::execute_func_name;

void godex::DynamicSystemInfo::get_info(DynamicSystemInfo &p_info, func_system_execute p_exec, SystemExeInfo &r_out) {
	// Assume is invalid.
	r_out.valid = false;

	// Validate.
	// Script function.
	ERR_FAIL_COND_MSG(p_info.target_script == nullptr, "[FATAL] This system doesn't have target assigned.");

	for (uint32_t i = 0; i < p_info.fetchers.size(); i += 1) {
		p_info.fetchers[i]->get_system_info(&r_out);
	}

	r_out.system_func = p_exec;

	// Arrived here, we can assume the system is valid.
	r_out.valid = true;
}

void godex::DynamicSystemInfo::executor(World *p_world, DynamicSystemInfo &p_info) {
	CRASH_COND_MSG(p_info.compiled == false, "The query is not supposed to be executed without being compiled.");

	// Script function.
	ERR_FAIL_COND_MSG(p_info.target_script == nullptr, "[FATAL] This system doesn't have target assigned.");

	// First extract the databags.
	for (uint32_t i = 0; i < p_info.fetchers.size(); i += 1) {
		// Set the accessors pointers.
		p_info.fetchers[i]->begin(p_world);
	}

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
				execute_func_name,
				const_cast<const Variant **>(p_info.access_ptr.ptr()),
				p_info.access_ptr.size(),
				err);
	}

	for (uint32_t i = 0; i < p_info.fetchers.size(); i += 1) {
		p_info.fetchers[i]->end();
	}

	ERR_FAIL_COND_MSG(err.error != Callable::CallError::CALL_OK, "System function execution error: " + itos(err.error) + " System name: " + ECS::get_system_name(p_info.system_id) + ". Please check the parameters.");
}
