/** @author AndreaCatania */

#pragma once

#include "../storages/storage.h"
#include "../world/world.h"
#include <tuple>

// ~~ Utility to remove the filter from the query. ~~
template <typename T>
struct remove_filter {
	typedef T type;
};

template <template <typename> class X, typename T>
struct remove_filter<X<T>> {
	typedef T type;
};

template <typename T>
using remove_filter_t = typename remove_filter<T>::type;

// ~~ Query filters. ~~
template <class C>
struct Without {};

template <class C>
struct Maybe {};

// ~~ Query storage, is used to fetch a single data. ~~

/// `QueryStorage` specialization with 0 template arguments.
template <class... Cs>
class QueryStorage {
public:
	QueryStorage(World *p_world) {}

	bool has_data(EntityID p_entity) const { return true; }
	std::tuple<Cs *...> get(EntityID p_id) const { return std::tuple(); }

	static void get_components(LocalVector<uint32_t> &r_mutable_components, LocalVector<uint32_t> &r_immutable_components) {}
};

/// `QueryStorage` `Maybe` filter specialization.
template <class C, class... Cs>
class QueryStorage<Maybe<C>, Cs...> : QueryStorage<Cs...> {
	TypedStorage<C> *storage = nullptr;

public:
	QueryStorage(World *p_world) :
			QueryStorage<Cs...>(p_world),
			storage(p_world->get_storage<C>()) {
	}

	bool has_data(EntityID p_entity) const {
		// The `Maybe` filter never stops the execution.
		return QueryStorage<Cs...>::has_data(p_entity);
	}

	std::tuple<C *, remove_filter_t<Cs> *...> get(EntityID p_id) const {
		if (likely(storage != nullptr) && storage->has(p_id)) {
			C *c = static_cast<C *>(storage->get_ptr(p_id));
			return std::tuple_cat(std::tuple<C *>(c), QueryStorage<Cs...>::get(p_id));
		} else {
			// Nothing to fetch, just set nullptr.
			return std::tuple_cat(std::tuple<C *>(nullptr), QueryStorage<Cs...>::get(p_id));
		}
	}

	static void get_components(LocalVector<uint32_t> &r_mutable_components, LocalVector<uint32_t> &r_immutable_components) {
		// The `WithoutFilter` collects the data always immutable.
		r_immutable_components.push_back(C::get_component_id());

		QueryStorage<Cs...>::get_components(r_mutable_components, r_immutable_components);
	}
};

/// `QueryStorage` `With` filter specialization.
template <class C, class... Cs>
class QueryStorage<Without<C>, Cs...> : QueryStorage<Cs...> {
	TypedStorage<C> *storage = nullptr;

public:
	QueryStorage(World *p_world) :
			QueryStorage<Cs...>(p_world),
			storage(p_world->get_storage<C>()) {
	}

	bool has_data(EntityID p_entity) const {
		if (unlikely(storage == nullptr)) {
			// When the storage is null the `Without` is always `true`.
			return true;
		}
		return storage->has(p_entity) == false && QueryStorage<Cs...>::has_data(p_entity);
	}

	std::tuple<C *, remove_filter_t<Cs> *...> get(EntityID p_id) const {
		// Just keep going, the `Without` filter doesn't collect data.
		return std::tuple_cat(std::tuple<C *>(nullptr), QueryStorage<Cs...>::get(p_id));
	}

	static void get_components(LocalVector<uint32_t> &r_mutable_components, LocalVector<uint32_t> &r_immutable_components) {
		// The `WithoutFilter` collects the data always immutable.
		r_immutable_components.push_back(C::get_component_id());

		QueryStorage<Cs...>::get_components(r_mutable_components, r_immutable_components);
	}
};

/// `QueryStorage` no filter specialization.
template <class C, class... Cs>
class QueryStorage<C, Cs...> : QueryStorage<Cs...> {
	TypedStorage<C> *storage = nullptr;

public:
	QueryStorage(World *p_world) :
			QueryStorage<Cs...>(p_world),
			storage(p_world->get_storage<C>()) {
	}

	bool has_data(EntityID p_entity) const {
		if (unlikely(storage == nullptr)) {
			return false;
		}
		return storage->has(p_entity) && QueryStorage<Cs...>::has_data(p_entity);
	}

	std::tuple<C *, remove_filter_t<Cs> *...> get(EntityID p_id) const {
#ifdef DEBUG_ENABLED
		// This can't happen because `is_done` returns true.
		CRASH_COND_MSG(storage == nullptr, "The storage" + String(typeid(TypedStorage<C>).name()) + " is null.");
#endif

		C *c = static_cast<C *>(storage->get_ptr(p_id));

		return std::tuple_cat(std::tuple<C *>(c), QueryStorage<Cs...>::get(p_id));
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

	QueryStorage<Cs...> q;

public:
	Query(World *p_world) :
			world(p_world), q(p_world) {
		// Prepare the query: advances to the first available entity.
		id = 0;
		if (q.has_data(0) == false) {
			next();
		}
	}

	bool is_done() const {
		return id == UINT32_MAX;
	}

	EntityID get_current_entity() const {
		return id;
	}

	void operator+=(uint32_t p_i) {
		for (uint32_t i = 0; i < p_i; i += 1) {
			next();
			if (is_done()) {
				break;
			}
		}
	}

	void next() {
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
	std::tuple<remove_filter_t<Cs> *...> get() const {
		CRASH_COND_MSG(id == UINT32_MAX, "No entities! Please use `is_done` to correctly stop the system execution.");
		return q.get(id);
	}

	static void get_components(LocalVector<uint32_t> &r_mutable_components, LocalVector<uint32_t> &r_immutable_components) {
		QueryStorage<Cs...>::get_components(r_mutable_components, r_immutable_components);
	}
};
