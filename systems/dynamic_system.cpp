#include "dynamic_system.h"

// gen
// TODO generate in a separate files, 200 elements of this.
#define DYNAMIC_SYSTEMS_MAX 1
uint32_t registered_dynamic_system_count = 0;
godex::DynamicSystemInfo dynamic_info[DYNAMIC_SYSTEMS_MAX];

void dynamic_system_internal_0(World *p_world) {
	godex::DynamicSystemInfo::executor(p_world, dynamic_info[0]);
}

system_execute dynamic_systems_ptr[DYNAMIC_SYSTEMS_MAX] = {
	dynamic_system_internal_0,
};

system_execute godex::register_dynamic_system(const DynamicSystemInfo &p_info) {
	const uint32_t id = registered_dynamic_system_count++;
	CRASH_COND_MSG(id >= DYNAMIC_SYSTEMS_MAX, "You can't register more than " + itos(DYNAMIC_SYSTEMS_MAX) + " dynamic systems. Please open an issue so we can increase this limit.");
	dynamic_info[id] = p_info;
	return dynamic_systems_ptr[id];
}
// ~gen

godex::DynamicSystemInfo::DynamicSystemInfo() {}

void godex::DynamicSystemInfo::set_target(Object *p_target) {
	target = p_target;
}

void godex::DynamicSystemInfo::with_resource(uint32_t p_resource_id, bool p_mutable) {
	const uint32_t index = resource_element_map.size() + query_element_map.size();
	resource_element_map.push_back(index);
	resources.push_back({ p_resource_id, p_mutable });
}

void godex::DynamicSystemInfo::with_component(uint32_t p_component_id, bool p_mutable) {
	const uint32_t index = resource_element_map.size() + query_element_map.size();
	query_element_map.push_back(index);
	query.add_component(p_component_id, p_mutable);
}

void godex::DynamicSystemInfo::without_component(uint32_t p_component_id) {
	CRASH_NOW_MSG("TODO implement this please.");
}

SystemInfo godex::DynamicSystemInfo::get_system_info() const {
	SystemInfo info;
	ERR_FAIL_COND_V(query.is_valid() == false, info);

	for (uint32_t i = 0; i < resources.size(); i += 1) {
		if (resources[i].is_mutable) {
			info.mutable_resources.push_back(resources[i].resource_id);
		} else {
			info.immutable_resources.push_back(resources[i].resource_id);
		}
	}

	query.get_system_info(info);
	return info;
}

StringName godex::DynamicSystemInfo::for_each_name;

void godex::DynamicSystemInfo::executor(World *p_world, DynamicSystemInfo &p_info) {
	ERR_FAIL_COND_MSG(p_info.target == nullptr, "[FATAL] This system doesn't have target assigned.");

	// Create the array where the storages are hold.
	LocalVector<Variant> access;
	LocalVector<Variant *> access_ptr;
	access.resize(p_info.resource_element_map.size() + p_info.query_element_map.size());
	access_ptr.resize(access.size());

	// First extract the resources.
	for (uint32_t i = 0; i < p_info.resources.size(); i += 1) {
		CRASH_NOW_MSG("TODO implement");
		// TODO get the resource
		//p_world->get_resource(p_info.resources[i].resource_id);
		// TODO map to aceess
	}

	// Map the query components, so the function can be called with the
	// right parameter order.
	for (uint32_t c = 0; c < p_info.query.access_count(); c += 1) {
		access[p_info.query_element_map[c]] = p_info.query.get_access(c);
		access_ptr[p_info.query_element_map[c]] = &access[p_info.query_element_map[c]];
	}

	p_info.query.begin(p_world);
	for (; p_info.query.is_done() == false; p_info.query.next_entity()) {
		Callable::CallError err;
		p_info.target->call(for_each_name, const_cast<const Variant **>(access_ptr.ptr()), access_ptr.size(), err);
		if (err.error != Callable::CallError::CALL_OK) {
			p_info.query.end();
			ERR_FAIL_COND(err.error != Callable::CallError::CALL_OK);
		}
	}
	p_info.query.end();
}
