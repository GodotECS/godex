#include "dynamic_query.h"

#include "../ecs.h"
#include "../modules/godot/nodes/ecs_world.h"

using godex::DynamicQuery;

void DynamicQuery::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_space", "space"), &DynamicQuery::set_space);
	ClassDB::bind_method(D_METHOD("with_component", "component_id", "mutable"), &DynamicQuery::with_component);
	ClassDB::bind_method(D_METHOD("maybe_component", "component_id", "mutable"), &DynamicQuery::maybe_component);
	ClassDB::bind_method(D_METHOD("changed_component", "component_id", "mutable"), &DynamicQuery::changed_component);
	ClassDB::bind_method(D_METHOD("not_component", "component_id"), &DynamicQuery::not_component);

	ClassDB::bind_method(D_METHOD("is_valid"), &DynamicQuery::is_valid);
	ClassDB::bind_method(D_METHOD("build"), &DynamicQuery::build);
	ClassDB::bind_method(D_METHOD("reset"), &DynamicQuery::reset);
	ClassDB::bind_method(D_METHOD("get_component", "index"), &DynamicQuery::get_access_by_index_gd);

	ClassDB::bind_method(D_METHOD("begin", "world"), &DynamicQuery::begin_script);
	ClassDB::bind_method(D_METHOD("end"), &DynamicQuery::end);

	ClassDB::bind_method(D_METHOD("next"), &DynamicQuery::next);

	ClassDB::bind_method(D_METHOD("has", "entity_index"), &DynamicQuery::script_has);
	ClassDB::bind_method(D_METHOD("fetch", "entity_index"), &DynamicQuery::script_fetch);

	ClassDB::bind_method(D_METHOD("get_current_entity_id"), &DynamicQuery::script_get_current_entity_id);
	ClassDB::bind_method(D_METHOD("count"), &DynamicQuery::count);
}

DynamicQuery::DynamicQuery() {
}

void DynamicQuery::set_space(Space p_space) {
	space = p_space;
}

void DynamicQuery::with_component(uint32_t p_component_id, bool p_mutable) {
	_with_component(p_component_id, p_mutable, WITH_MODE);
}

void DynamicQuery::maybe_component(uint32_t p_component_id, bool p_mutable) {
	_with_component(p_component_id, p_mutable, MAYBE_MODE);
}

void DynamicQuery::changed_component(uint32_t p_component_id, bool p_mutable) {
	_with_component(p_component_id, p_mutable, CHANGED_MODE);
}

void DynamicQuery::not_component(uint32_t p_component_id) {
	_with_component(p_component_id, false, WITHOUT_MODE);
}

void DynamicQuery::_with_component(uint32_t p_component_id, bool p_mutable, FetchMode p_mode) {
	ERR_FAIL_COND_MSG(is_valid() == false, "This query is not valid.");
	ERR_FAIL_COND_MSG(can_change == false, "This query can't change at this point, you have to `clear` it.");
	if (unlikely(ECS::verify_component_id(p_component_id) == false)) {
		// Invalidate.
		valid = false;
		ERR_FAIL_MSG("The component_id " + itos(p_component_id) + " is invalid.");
	}

	ERR_FAIL_COND_MSG(component_ids.find(p_component_id) != -1, "The component " + itos(p_component_id) + " is already part of this query.");

	component_ids.push_back(p_component_id);
	mutability.push_back(p_mutable);
	mode.push_back(p_mode);
}

bool DynamicQuery::is_valid() const {
	return valid;
}

bool DynamicQuery::build() {
	if (likely(can_change == false)) {
		return false;
	}
	ERR_FAIL_COND_V(is_valid() == false, false);

	can_change = false;

	// Build the access_component in this way the `ObjectDB` doesn't
	// complain for some reason, otherwise it needs to use pointers
	// (AccessComponent is a parent of Object).
	accessors.resize(component_ids.size());
	for (uint32_t i = 0; i < component_ids.size(); i += 1) {
		accessors[i].init(
				component_ids[i],
				mutability[i]);
	}

	return true;
}

void DynamicQuery::reset() {
	valid = true;
	can_change = true;
	component_ids.clear();
	mutability.clear();
	accessors.clear();
	world = nullptr;
}

uint32_t DynamicQuery::access_count() const {
	return component_ids.size();
}

Object *DynamicQuery::get_access_by_index_gd(uint32_t p_index) const {
	ERR_FAIL_COND_V_MSG(is_valid() == false, nullptr, "The query is invalid.");
	return (Object *)(accessors.ptr() + p_index);
}

ComponentDynamicExposer *DynamicQuery::get_access_by_index(uint32_t p_index) const {
	ERR_FAIL_COND_V_MSG(is_valid() == false, nullptr, "The query is invalid.");
	return (ComponentDynamicExposer *)(accessors.ptr() + p_index);
}

void DynamicQuery::get_system_info(SystemExeInfo *p_info) const {
	ERR_FAIL_COND(is_valid() == false);
	for (uint32_t i = 0; i < component_ids.size(); i += 1) {
		if (mutability[i]) {
			p_info->mutable_components.insert(component_ids[i]);
		} else {
			p_info->immutable_components.insert(component_ids[i]);
		}

		if (mode[i] == CHANGED_MODE) {
			p_info->need_changed.insert(component_ids[i]);
		}
	}
}

void DynamicQuery::begin_script(Object *p_world) {
	WorldECS *world = Object::cast_to<WorldECS>(p_world);
	ERR_FAIL_COND_MSG(world == nullptr, "The given object is not a `WorldECS`.");
	begin(world->get_world());
}

void DynamicQuery::begin(World *p_world) {
	// Can't change anymore.
	build();
	current_entity = EntityID();
	iterator_index = 0;
	entities.count = 0;

	ERR_FAIL_COND(is_valid() == false);

	// Using a crash cond so the dev knows immediately if it's using the query
	// in the wrong way.
	// The user doesn't never use this query directly. So it's fine put the
	// crash cond here.
	CRASH_COND_MSG(world != nullptr, "Make sure to call `DynamicQuery::end()` when you finish using the query!");
	world = p_world;

	storages.resize(component_ids.size());
	entities.count = UINT32_MAX;

	for (uint32_t i = 0; i < component_ids.size(); i += 1) {
		storages[i] = world->get_storage(component_ids[i]);
		if (storages[i] != nullptr) {
			EntitiesBuffer eb(UINT32_MAX, nullptr);
			switch (mode[i]) {
				case WITH_MODE: {
					eb = storages[i]->get_stored_entities();
				} break;
				case WITHOUT_MODE: {
					// Not determinant, nothing to do.
				} break;
				case MAYBE_MODE: {
					// Not determinant, nothing to do.
				} break;
				case CHANGED_MODE: {
					eb = storages[i]->get_changed_entities();
				} break;
			}
			if (eb.count < entities.count) {
				entities = eb;
			}
		}
	}

	if (unlikely(entities.count == UINT32_MAX)) {
		entities.count = 0;
		valid = false;
		ERR_PRINT("The Query can't be used if there are only non determinant filters (like `Without` and `Maybe`).");
	}

	// The Query is ready to fetch, let's rock!
}

void DynamicQuery::end() {
	// Clear any component reference.
	world = nullptr;
	storages.clear();
	iterator_index = 0;
	entities.count = 0;
}

bool DynamicQuery::next() {
	// Search the next Entity to fetch.
	while (iterator_index < entities.count) {
		const EntityID entity_id = entities.entities[iterator_index];
		iterator_index += 1;

		if (has(entity_id)) {
			fetch(entity_id);
			return true;
		}
	}

	// Nothing more to fetch.
	return false;
}

bool DynamicQuery::script_has(uint32_t p_id) const {
	return has(p_id);
}

bool DynamicQuery::has(EntityID p_id) const {
	// Make sure this entity has all the following components.
	uint32_t non_determinant_count = 0;
	for (uint32_t i = 0; i < storages.size(); i += 1) {
		switch (mode[i]) {
			case WITH_MODE: {
				if (unlikely(storages[i] == nullptr) || storages[i]->has(p_id) == false) {
					// Nothing.
					return false;
				}
			} break;
			case WITHOUT_MODE: {
				if (unlikely(storages[i] != nullptr) && storages[i]->has(p_id)) {
					// Without is the opposite of `WITH`.
					return false;
				}
				non_determinant_count += 1;
			} break;
			case MAYBE_MODE: {
				// Maybe returns always true.
				non_determinant_count += 1;
			} break;
			case CHANGED_MODE: {
				if (unlikely(storages[i] == nullptr) || storages[i]->is_changed(p_id) == false) {
					// Returns false if the storage doesn't exists or is not
					// changed.
					return false;
				}
			} break;
		}
	}

	ERR_FAIL_COND_V_MSG(
			non_determinant_count == storages.size(),
			false,
			"Fetching using only non_determinant filters (without and maybe) is not allowed. The reason is simply becayse the result is just meaningless.");

	// This entity can be fetched.
	return true;
}

void DynamicQuery::script_fetch(uint32_t p_entity_id) {
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND_MSG(has(p_entity_id) == false, "[FATAL] This entity " + itos(p_entity_id) + " can't be fetched by this query. Please check it using the functin `has`.");
#endif
	fetch(p_entity_id);
}

void DynamicQuery::fetch(EntityID p_entity_id) {
	for (uint32_t i = 0; i < storages.size(); i += 1) {
		if (storages[i] != nullptr && storages[i]->has(p_entity_id)) {
			if (accessors[i].is_mutable()) {
				accessors[i].set_target(storages[i]->get_ptr(p_entity_id, space));
			} else {
				// Taken using the **CONST** `get_ptr` function, but casted back
				// to mutable. The `Accessor` already guards its accessibility
				// so it's safe do so.
				// Note: this is used by GDScript, we don't need that this is
				// const at compile time.
				// Note: since we have to storage mutable, it's safe cast this
				// data back to mutable.
				// Note: `std::as_const` doesn't work here. The compile is
				// optimizing it? Well, I'm just using `const_cast`.
				const void *c(const_cast<const StorageBase *>(storages[i])->get_ptr(p_entity_id, space));
				accessors[i].set_target(const_cast<void *>(c));
			}
		} else {
			// This data not found, just set nullptr.
			accessors[i].set_target(nullptr);
		}
	}
	current_entity = p_entity_id;
}

uint32_t DynamicQuery::script_get_current_entity_id() const {
	return get_current_entity_id();
}

EntityID DynamicQuery::get_current_entity_id() const {
	return current_entity;
}

uint32_t DynamicQuery::count() const {
	uint32_t count = 0;
	for (uint32_t i = 0; i < entities.count; i += 1) {
		if (has(entities.entities[i])) {
			count += 1;
		}
	}
	return count;
}

void DynamicQuery::setvar(const Variant &p_key, const Variant &p_value, bool *r_valid) {
	*r_valid = true;
	// Assume valid, nothing to do.
}

Variant DynamicQuery::getvar(const Variant &p_key, bool *r_valid) const {
	if (p_key.get_type() == Variant::INT) {
		Object *obj = get_access_by_index_gd(p_key);
		if (obj == nullptr) {
			*r_valid = false;
			return Variant();
		} else {
			*r_valid = true;
			return obj;
		}
	} else {
		*r_valid = false;
		ERR_PRINT("The proper syntax is: `query[0].my_component_variable`.");
		return Variant();
	}
}