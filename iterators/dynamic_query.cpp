#include "dynamic_query.h"

#include "modules/ecs/ecs.h"

using godex::AccessComponent;
using godex::DynamicQuery;

void AccessComponent::set_mutable(bool p_mut) {
	mut = p_mut;
}

AccessComponent::AccessComponent() {}

bool AccessComponent::_setv(const StringName &p_name, const Variant &p_data) {
	ERR_FAIL_COND_V_MSG(mut == false, false, "This component was taken as not mutable.");
	return component->set(p_name, p_data);
}

bool AccessComponent::_getv(const StringName &p_name, Variant &r_data) const {
	return component->get(p_name, r_data);
}

bool AccessComponent::is_mutable() const {
	return mut;
}

DynamicQuery::DynamicQuery() {
}

void DynamicQuery::add_component(uint32_t p_component_id, bool p_mutable) {
	ERR_FAIL_COND_MSG(is_valid() == false, "This query is not valid.");
	ERR_FAIL_COND_MSG(can_change == false, "This query can't change at this point, you have to `clear` it.");
	if (unlikely(ECS::verify_component_id(p_component_id) == false)) {
		// Invalidate.
		valid = false;
		ERR_FAIL_MSG("The component_id " + itos(p_component_id) + " is invalid.");
	}

	component_ids.push_back(p_component_id);
	mutability.push_back(p_mutable);
}

bool DynamicQuery::is_valid() const {
	return valid;
}

bool DynamicQuery::build() {
	ERR_FAIL_COND_V(is_valid() == false, false);
	if (likely(can_change == false)) {
		return false;
	}

	can_change = false;

	// Build the access_component is this way doesn't make the `ObjectDB`
	// complain for some reason.
	access_components.resize(component_ids.size());
	for (uint32_t i = 0; i < component_ids.size(); i += 1) {
		access_components[i].set_mutable(mutability[i]);
	}

	return true;
}

void DynamicQuery::reset() {
	valid = true;
	can_change = true;
	component_ids.clear();
	mutability.clear();
	access_components.clear();
	world = nullptr;
}

uint32_t DynamicQuery::access_count() const {
	return component_ids.size();
}

AccessComponent *DynamicQuery::get_access(uint32_t p_index) {
	ERR_FAIL_COND_V_MSG(is_valid() == false, nullptr, "The query is invalid.");
	build();
	return access_components.ptr() + p_index;
}

void DynamicQuery::begin(World *p_world) {
	// Can't change anymore.
	build();
	entity_id = UINT32_MAX;

	ERR_FAIL_COND(is_valid() == false);

	// Using a crash cond so the dev knows immediately if it's using the query
	// in the wrong way.
	// The user doesn't never use this query directly. So it's fine put the
	// crash cond here.
	CRASH_COND_MSG(world != nullptr, "Make sure to call `DynamicQuery::end()` when you finish using the query!");
	world = p_world;

	storages.resize(component_ids.size());
	for (uint32_t i = 0; i < component_ids.size(); i += 1) {
		storages[i] = world->get_storage(component_ids[i]);
		ERR_FAIL_COND_MSG(storages[i] == nullptr, "There is a storage nullptr. This is not supposed to happen.");
	}

	// At this point all the storages are taken

	// Search the fist entity
	entity_id = 0;
	if (has_entity(0) == false) {
		next_entity();
	} else {
		fetch();
	}
}

bool DynamicQuery::is_done() const {
	return entity_id == UINT32_MAX;
}

EntityID DynamicQuery::get_current_entity_id() const {
	return entity_id;
}

// TODO see how to improve this lookup mechanism so that no cache is miss and
// it's fast.
void DynamicQuery::next_entity() {
	const uint32_t last_id = world->get_last_entity_id();
	if (unlikely(entity_id == UINT32_MAX || last_id == UINT32_MAX)) {
		entity_id = UINT32_MAX;
		return;
	}

	for (uint32_t new_entity_id = entity_id + 1; new_entity_id <= last_id; new_entity_id += 1) {
		if (has_entity(new_entity_id)) {
			// Confirmed, this `new_entity_id` has all the storages.
			entity_id = new_entity_id;
			fetch();
			return;
		}
	}

	// No more entity
	entity_id = UINT32_MAX;
}

void DynamicQuery::end() {
	// Clear any component reference.
	for (uint32_t i = 0; i < component_ids.size(); i += 1) {
		access_components[i].component = nullptr;
	}

	world = nullptr;
	storages.clear();
}

void DynamicQuery::get_system_info(SystemInfo &p_info) const {
	ERR_FAIL_COND(is_valid() == false);
	for (uint32_t i = 0; i < component_ids.size(); i += 1) {
		if (mutability[i]) {
			p_info.mutable_components.push_back(component_ids[i]);
		} else {
			p_info.immutable_components.push_back(component_ids[i]);
		}
	}
}

bool DynamicQuery::has_entity(EntityID p_id) const {
	for (uint32_t i = 0; i < storages.size(); i += 1) {
		if (storages[i]->has(p_id) == false) {
			return false;
		}
	}
	return true;
}

void DynamicQuery::fetch() {
	ERR_FAIL_COND_MSG(entity_id == UINT32_MAX, "There is nothing to fetch.");

	for (uint32_t i = 0; i < storages.size(); i += 1) {
		access_components[i].component = storages[i]->get_ptr(entity_id);
	}
}
