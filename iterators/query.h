/** @author AndreaCatania */

#pragma once

#include "modules/ecs/storages/storage.h"
#include "modules/ecs/world/world.h"
#include <tuple>

template <class... Cs>
class QueryStorage {
public:
	QueryStorage(World *p_world) {}

	bool has_data(EntityID p_entity) const { return true; }
	std::tuple<Cs &...> get(EntityID p_id) const { return std::tuple(); }

	static void get_components(LocalVector<uint32_t> &r_mutable_components, LocalVector<uint32_t> &r_immutable_components) {}
};

template <class C, class... Cs>
class QueryStorage<C, Cs...> : QueryStorage<Cs...> {
	TypedStorage<C> *storage = nullptr;

public:
	QueryStorage(World *p_world) :
			QueryStorage<Cs...>(p_world) {
		storage = p_world->get_storage<C>();
		ERR_FAIL_COND_MSG(storage == nullptr, "The storage" + String(typeid(TypedStorage<C>).name()) + " is null.");
	}

	bool has_data(EntityID p_entity) const {
		if (storage == nullptr) {
			return false;
		}
		return storage->has(p_entity) && QueryStorage<Cs...>::has_data(p_entity);
	}

	std::tuple<C &, Cs &...> get(EntityID p_id) const {
#ifdef DEBUG_ENABLED
		// This can't happen because `is_done` returns true.
		CRASH_COND_MSG(storage == nullptr, "The storage" + String(typeid(TypedStorage<C>).name()) + " is null.");
#endif

		C &c = storage->get(p_id);

		return std::tuple_cat(std::tuple<C &>(c), QueryStorage<Cs...>::get(p_id));
	}

	static void get_components(LocalVector<uint32_t> &r_mutable_components, LocalVector<uint32_t> &r_immutable_components) {
		if (std::is_const<C>()) {
			r_immutable_components.push_back(C::get_component_id());
		} else {
			r_mutable_components.push_back(C::get_component_id());
		}

		QueryStorage<Cs...>::get_components(r_mutable_components, r_immutable_components);
	}
};

/// This is the fastest `Query`.
/// Using the variadic template, it's build at compile time. Since the
/// components must be known at compile time, this query can't by used by
/// scripts that have to rely on the `DynamicQuery`.
template <class... Cs>
class Query {
	World *world;
	uint32_t id = UINT32_MAX;

	QueryStorage<std::remove_reference_t<Cs>...> q;

public:
	Query(World *p_world) :
			world(p_world), q(p_world) {
		// Prepare the query: advances to the first available entity.
		id = 0;
		if (q.has_data(0) == false) {
			next_entity();
		}
	}

	bool is_done() const {
		return id == UINT32_MAX;
	}

	void operator+=(uint32_t p_i) {
		for (uint32_t i = 0; i < p_i; i += 1) {
			next_entity();
			if (is_done()) {
				break;
			}
		}
	}

	void next_entity() {
		const uint32_t last_id = world->get_last_entity_id();
		if (unlikely(id == UINT32_MAX || last_id == UINT32_MAX)) {
			id = UINT32_MAX;
			return;
		}

		for (uint32_t i = id + 1; i <= last_id; i += 1) {
			if (q.has_data(i)) {
				// This is the next entity that fulfils the query.
				id = i;
				return;
			}
		}

		// No more entity
		id = UINT32_MAX;
	}

	// TODO The lockup mechanism of this query must be improved to avoid any
	// useless operation.
	std::tuple<std::remove_reference_t<Cs> &...> get() const {
		CRASH_COND_MSG(id == UINT32_MAX, "No entities! Please use `is_done` to correctly stop the system execution.");
		return q.get(id);
	}

	static void get_components(LocalVector<uint32_t> &r_mutable_components, LocalVector<uint32_t> &r_immutable_components) {
		QueryStorage<std::remove_reference_t<Cs>...>::get_components(r_mutable_components, r_immutable_components);
	}
};
