#include "dynamic_query.h"

#include "../ecs.h"

using godex::DynamicQuery;

void DynamicQuery::_bind_methods() {
	ClassDB::bind_method(D_METHOD("with_component", "component_id", "mutable"), &DynamicQuery::with_component);
	ClassDB::bind_method(D_METHOD("maybe_component", "component_id", "mutable"), &DynamicQuery::maybe_component);
	ClassDB::bind_method(D_METHOD("without_component", "component_id"), &DynamicQuery::without_component);
	ClassDB::bind_method(D_METHOD("is_valid"), &DynamicQuery::is_valid);
	ClassDB::bind_method(D_METHOD("build"), &DynamicQuery::build);
	ClassDB::bind_method(D_METHOD("reset"), &DynamicQuery::reset);
	ClassDB::bind_method(D_METHOD("get_component", "index"), &DynamicQuery::get_access);
	ClassDB::bind_method(D_METHOD("begin", "world"), &DynamicQuery::begin_script);
	ClassDB::bind_method(D_METHOD("is_done"), &DynamicQuery::is_done);
	ClassDB::bind_method(D_METHOD("get_current_entity_id"), &DynamicQuery::get_current_entity_id_script);
	ClassDB::bind_method(D_METHOD("next"), &DynamicQuery::next);
	ClassDB::bind_method(D_METHOD("end"), &DynamicQuery::end);
}

DynamicQuery::DynamicQuery() {
}

void DynamicQuery::with_component(uint32_t p_component_id, bool p_mutable) {
	_with_component(p_component_id, p_mutable, true);
}

void DynamicQuery::maybe_component(uint32_t p_component_id, bool p_mutable) {
	_with_component(p_component_id, p_mutable, false);
}

void DynamicQuery::without_component(uint32_t p_component_id) {
	ERR_FAIL_COND_MSG(is_valid() == false, "This query is not valid.");
	ERR_FAIL_COND_MSG(can_change == false, "This query can't change at this point, you have to `clear` it.");
	if (unlikely(ECS::verify_component_id(p_component_id) == false)) {
		// Invalidate.
		valid = false;
		ERR_FAIL_MSG("The component_id " + itos(p_component_id) + " is invalid.");
	}

	ERR_FAIL_COND_MSG(component_ids.find(p_component_id) != -1, "The component " + itos(p_component_id) + " is already part of this query.");
	ERR_FAIL_COND_MSG(reject_component_ids.find(p_component_id) != -1, "The component " + itos(p_component_id) + " is already part of this query.");

	reject_component_ids.push_back(p_component_id);
}

void DynamicQuery::_with_component(uint32_t p_component_id, bool p_mutable, bool p_required) {
	ERR_FAIL_COND_MSG(is_valid() == false, "This query is not valid.");
	ERR_FAIL_COND_MSG(can_change == false, "This query can't change at this point, you have to `clear` it.");
	if (unlikely(ECS::verify_component_id(p_component_id) == false)) {
		// Invalidate.
		valid = false;
		ERR_FAIL_MSG("The component_id " + itos(p_component_id) + " is invalid.");
	}

	ERR_FAIL_COND_MSG(component_ids.find(p_component_id) != -1, "The component " + itos(p_component_id) + " is already part of this query.");
	ERR_FAIL_COND_MSG(reject_component_ids.find(p_component_id) != -1, "The component " + itos(p_component_id) + " is already part of this query.");

	component_ids.push_back(p_component_id);
	mutability.push_back(p_mutable);
	required.push_back(p_required);
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

	// Build the access_component in this way the `ObjectDB` doesn't
	// complain for some reason, otherwise it needs to use pointers
	// (AccessComponent is a parent of Object).
	accessors.resize(component_ids.size());
	accessors_obj.resize(component_ids.size());
	for (uint32_t i = 0; i < component_ids.size(); i += 1) {
		accessors[i].__mut = mutability[i];

		// The function `set_script_and_instance` is the only way to set a
		// script instance with lifetime handled by us. The only requirement is
		// submit a pointer to a `Script`, though we can cheat :P
		const Variant pointer_to_anything = &accessors_obj[i];
		accessors_obj[i].set_script_and_instance(pointer_to_anything, &accessors[i]);
	}

	return true;
}

void DynamicQuery::reset() {
	valid = true;
	can_change = true;
	component_ids.clear();
	mutability.clear();
	accessors.clear();
	accessors_obj.clear();
	world = nullptr;
}

uint32_t DynamicQuery::access_count() const {
	return component_ids.size();
}

Object *DynamicQuery::get_access(uint32_t p_index) {
	ERR_FAIL_COND_V_MSG(is_valid() == false, nullptr, "The query is invalid.");
	build();
	return accessors_obj.ptr() + p_index;
}

void DynamicQuery::begin_script(Object *p_world) {
	World *world = godex::AccessResource::unwrap<World>(p_world);
	ERR_FAIL_COND_MSG(world == nullptr, "The given object is not a `World` `Resource`.");
	begin(world);
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
		if (unlikely(storages[i] == nullptr)) {
			// The query can end now because there is an entire not used storage.
			entity_id = UINT32_MAX;
			return;
		}
	}

	reject_storages.resize(reject_component_ids.size());
	for (uint32_t i = 0; i < reject_component_ids.size(); i += 1) {
		reject_storages[i] = world->get_storage(reject_component_ids[i]);
		// `reject_storage` can be `nullptr`.
	}

	// At this point all the storages are taken.

	// Search the fist entity
	entity_id = 0;
	if (has_entity(0) == false) {
		next();
	} else {
		fetch();
	}
}

bool DynamicQuery::is_done() const {
	return entity_id == UINT32_MAX;
}

uint32_t DynamicQuery::get_current_entity_id_script() const {
	return entity_id;
}

EntityID DynamicQuery::get_current_entity_id() const {
	return entity_id;
}

// TODO see how to improve this lookup mechanism so that no cache is miss and
// it's fast.
void DynamicQuery::next() {
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
		accessors[i].__target = nullptr;
	}

	world = nullptr;
	storages.clear();
}

void DynamicQuery::get_system_info(SystemExeInfo &p_info) const {
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
	// Make sure this entity has all the following components.
	for (uint32_t i = 0; i < storages.size(); i += 1) {
		if (required[i] && storages[i]->has(p_id) == false) {
			// The component is not found and it's required.
			return false;
		}
	}

	// Make sure this entity DOESN'T have the following components.
	for (uint32_t i = 0; i < reject_storages.size(); i += 1) {
		if (likely(reject_storages[i]) && reject_storages[i]->has(p_id)) {
			// The component is found.
			return false;
		}
	}

	// This entity can be fetched.
	return true;
}

void DynamicQuery::fetch() {
	ERR_FAIL_COND_MSG(entity_id == UINT32_MAX, "There is nothing to fetch.");

	for (uint32_t i = 0; i < storages.size(); i += 1) {
		if (required[i] || storages[i]->has(entity_id)) {
			accessors[i].__target = storages[i]->get_ptr(entity_id);
		} else {
			// This data is not required and is not found.
			accessors[i].__target = nullptr;
		}
	}
}
