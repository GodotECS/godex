/** @author AndreaCatania */

#pragma once

#include "../storage/storage.h"
#include "../systems/system.h"
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

template <class C>
struct Changed {};

// ~~ Query storage, is used to fetch a single data. ~~

/// `QueryStorage` specialization with 0 template arguments.
template <class... Cs>
class QueryStorage {
public:
	QueryStorage(World *p_world) {}

	EntitiesBuffer get_entities() const {
		// This is a NON determinant filter, so just return UINT32_MAX.
		return { UINT32_MAX, nullptr };
	}

	bool has_data(EntityID p_entity) const { return true; }
	std::tuple<Batch<remove_filter_t<Cs>>...> get(EntityID p_id, Space p_mode) const { return std::tuple(); }

	static void get_components(SystemExeInfo &r_info) {}
};

/// Fetch the `EntityID`.
template <class... Cs>
class QueryStorage<const EntityID, Cs...> : QueryStorage<Cs...> {
	EntityID current_entity;

public:
	QueryStorage(World *p_world) :
			QueryStorage<Cs...>(p_world) {}

	EntitiesBuffer get_entities() const {
		return QueryStorage<Cs...>::get_entities();
	}

	bool has_data(EntityID p_entity) const {
		return QueryStorage<Cs...>::has_data(p_entity);
	}

	std::tuple<Batch<const EntityID>, Batch<remove_filter_t<Cs>>...> get(EntityID p_id, Space p_mode) {
		current_entity = p_id;
		return std::tuple_cat(
				std::tuple<Batch<const EntityID>>(&current_entity),
				QueryStorage<Cs...>::get(p_id, p_mode));
	}

	static void get_components(SystemExeInfo &r_info) {
		QueryStorage<Cs...>::get_components(r_info);
	}
};

/// `QueryStorage` `Maybe` mutable filter specialization.
template <class C, class... Cs>
class QueryStorage<Maybe<C>, Cs...> : QueryStorage<Cs...> {
	Storage<C> *storage = nullptr;

public:
	QueryStorage(World *p_world) :
			QueryStorage<Cs...>(p_world),
			storage(p_world->get_storage<C>()) {
	}

	EntitiesBuffer get_entities() const {
		// This is a NON determinant filter, so just return the other filter.
		return QueryStorage<Cs...>::get_entities();
	}

	bool has_data(EntityID p_entity) const {
		// The `Maybe` filter never stops the execution.
		return QueryStorage<Cs...>::has_data(p_entity);
	}

	std::tuple<Batch<C>, Batch<remove_filter_t<Cs>>...> get(EntityID p_id, Space p_mode) const {
		if (likely(storage != nullptr) && storage->has(p_id)) {
			return std::tuple_cat(
					std::tuple<Batch<C>>(storage->get(p_id, p_mode)),
					QueryStorage<Cs...>::get(p_id, p_mode));
		} else {
			// Nothing to fetch, just set nullptr.
			return std::tuple_cat(
					std::tuple<Batch<C>>(Batch<C>()),
					QueryStorage<Cs...>::get(p_id, p_mode));
		}
	}

	static void get_components(SystemExeInfo &r_info) {
		r_info.mutable_components.insert(C::get_component_id());
		QueryStorage<Cs...>::get_components(r_info);
	}
};

/// `QueryStorage` `Maybe` immutable filter specialization.
template <class C, class... Cs>
class QueryStorage<Maybe<const C>, Cs...> : QueryStorage<Cs...> {
	const Storage<const C> *storage = nullptr;

public:
	QueryStorage(World *p_world) :
			QueryStorage<Cs...>(p_world),
			storage(std::as_const(p_world)->get_storage<const C>()) {
	}

	EntitiesBuffer get_entities() const {
		// This is a NON determinant filter, so just return the other filter.
		return QueryStorage<Cs...>::get_entities();
	}

	bool has_data(EntityID p_entity) const {
		// The `Maybe` filter never stops the execution.
		return QueryStorage<Cs...>::has_data(p_entity);
	}

	std::tuple<Batch<const C>, Batch<remove_filter_t<Cs>>...> get(EntityID p_id, Space p_mode) const {
		if (likely(storage != nullptr) && storage->has(p_id)) {
			return std::tuple_cat(
					std::tuple<Batch<const C>>(storage->get(p_id, p_mode)),
					QueryStorage<Cs...>::get(p_id, p_mode));
		} else {
			// Nothing to fetch, just set nullptr.
			return std::tuple_cat(
					std::tuple<Batch<const C>>(Batch<const C>()),
					QueryStorage<Cs...>::get(p_id, p_mode));
		}
	}

	static void get_components(SystemExeInfo &r_info) {
		r_info.immutable_components.insert(C::get_component_id());
		QueryStorage<Cs...>::get_components(r_info);
	}
};

/// `QueryStorage` `With` filter specialization.
template <class C, class... Cs>
class QueryStorage<Without<C>, Cs...> : QueryStorage<Cs...> {
	Storage<C> *storage = nullptr;

public:
	QueryStorage(World *p_world) :
			QueryStorage<Cs...>(p_world),
			storage(p_world->get_storage<C>()) {
	}

	EntitiesBuffer get_entities() const {
		// This is a NON determinant filter, so just return the other filter.
		return QueryStorage<Cs...>::get_entities();
	}

	bool has_data(EntityID p_entity) const {
		if (unlikely(storage == nullptr)) {
			// When the storage is null the `Without` is always `true` though
			// we have to keep check the other storages.
			return QueryStorage<Cs...>::has_data(p_entity);
		}
		return storage->has(p_entity) == false && QueryStorage<Cs...>::has_data(p_entity);
	}

	std::tuple<Batch<C>, Batch<remove_filter_t<Cs>>...> get(EntityID p_id, Space p_mode) const {
		// Just keep going, the `Without` filter doesn't collect data.
		return std::tuple_cat(std::tuple<C *>(nullptr), QueryStorage<Cs...>::get(p_id, p_mode));
	}

	static void get_components(SystemExeInfo &r_info) {
		// The `Without` collects the data always immutable.
		r_info.immutable_components.insert(C::get_component_id());
		QueryStorage<Cs...>::get_components(r_info);
	}
};

/// `QueryStorage` `Changed` mutable filter specialization.
template <class C, class... Cs>
class QueryStorage<Changed<C>, Cs...> : QueryStorage<Cs...> {
	Storage<C> *storage = nullptr;

public:
	QueryStorage(World *p_world) :
			QueryStorage<Cs...>(p_world),
			storage(p_world->get_storage<C>()) {
	}

	EntitiesBuffer get_entities() const {
		// This is a determinant filter, that iterates over the changed
		// components of this storage.
		const EntitiesBuffer o_entities = QueryStorage<Cs...>::get_entities();
		if (unlikely(storage == nullptr)) {
			return o_entities;
		}
		const EntitiesBuffer entities = storage->get_changed_entities();
		return entities.count < o_entities.count ? entities : o_entities;
	}

	bool has_data(EntityID p_entity) const {
		if (unlikely(storage == nullptr)) {
			// This is a required field, since there is no storage this can end
			// immediately.
			return false;
		}
		return storage->is_changed(p_entity) && QueryStorage<Cs...>::has_data(p_entity);
	}

	std::tuple<Batch<C>, Batch<remove_filter_t<Cs>>...> get(EntityID p_id, Space p_mode) const {
#ifdef DEBUG_ENABLED
		// This can't happen because `is_done` returns true.
		CRASH_COND_MSG(storage == nullptr, "The storage" + String(typeid(Storage<C>).name()) + " is null.");
#endif

		return std::tuple_cat(
				std::tuple<Batch<C>>(storage->get(p_id, p_mode)),
				QueryStorage<Cs...>::get(p_id, p_mode));
	}

	static void get_components(SystemExeInfo &r_info) {
		r_info.mutable_components.insert(C::get_component_id());
		r_info.need_changed.insert(C::get_component_id());
		QueryStorage<Cs...>::get_components(r_info);
	}
};

/// `QueryStorage` `Changed` immutable filter specialization.
template <class C, class... Cs>
class QueryStorage<Changed<const C>, Cs...> : QueryStorage<Cs...> {
	const Storage<const C> *storage = nullptr;

public:
	QueryStorage(World *p_world) :
			QueryStorage<Cs...>(p_world),
			storage(std::as_const(p_world)->get_storage<const C>()) {
	}

	EntitiesBuffer get_entities() const {
		// This is a determinant filter, that iterates over the changed
		// components of this storage.
		const EntitiesBuffer o_entities = QueryStorage<Cs...>::get_entities();
		if (unlikely(storage == nullptr)) {
			return o_entities;
		}
		const EntitiesBuffer entities = storage->get_changed_entities();
		return entities.count < o_entities.count ? entities : o_entities;
	}

	bool has_data(EntityID p_entity) const {
		if (unlikely(storage == nullptr)) {
			// This is a required field, since there is no storage this can end
			// immediately.
			return false;
		}
		return storage->is_changed(p_entity) && QueryStorage<Cs...>::has_data(p_entity);
	}

	std::tuple<Batch<const C>, Batch<remove_filter_t<Cs>>...> get(EntityID p_id, Space p_mode) const {
#ifdef DEBUG_ENABLED
		// This can't happen because `is_done` returns true.
		CRASH_COND_MSG(storage == nullptr, "The storage" + String(typeid(Storage<C>).name()) + " is null.");
#endif

		return std::tuple_cat(
				std::tuple<Batch<const C>>(storage->get(p_id, p_mode)),
				QueryStorage<Cs...>::get(p_id, p_mode));
	}

	static void get_components(SystemExeInfo &r_info) {
		r_info.immutable_components.insert(C::get_component_id());
		r_info.need_changed.insert(C::get_component_id());
		QueryStorage<Cs...>::get_components(r_info);
	}
};

/// `QueryStorage` no filter specialization.
template <class C, class... Cs>
class QueryStorage<C, Cs...> : QueryStorage<Cs...> {
	Storage<C> *storage = nullptr;

public:
	QueryStorage(World *p_world) :
			QueryStorage<Cs...>(p_world),
			storage(p_world->get_storage<C>()) {
	}

	EntitiesBuffer get_entities() const {
		// This is a determinant filter, that iterates over the existing
		// components of this storage.
		const EntitiesBuffer o_entities = QueryStorage<Cs...>::get_entities();
		if (unlikely(storage == nullptr)) {
			return o_entities;
		}
		const EntitiesBuffer entities = storage->get_stored_entities();
		return entities.count < o_entities.count ? entities : o_entities;
	}

	bool has_data(EntityID p_entity) const {
		if (unlikely(storage == nullptr)) {
			// This is a required field, since there is no storage this can end
			// immediately.
			return false;
		}
		return storage->has(p_entity) && QueryStorage<Cs...>::has_data(p_entity);
	}

	std::tuple<Batch<C>, Batch<remove_filter_t<Cs>>...> get(EntityID p_id, Space p_mode) const {
#ifdef DEBUG_ENABLED
		// This can't happen because `is_done` returns true.
		CRASH_COND_MSG(storage == nullptr, "The storage" + String(typeid(Storage<C>).name()) + " is null.");
#endif

		return std::tuple_cat(
				std::tuple<Batch<C>>(storage->get(p_id, p_mode)),
				QueryStorage<Cs...>::get(p_id, p_mode));
	}

	static void get_components(SystemExeInfo &r_info) {
		r_info.mutable_components.insert(C::get_component_id());
		QueryStorage<Cs...>::get_components(r_info);
	}
};

/// `QueryStorage` const no filter specialization.
template <class C, class... Cs>
class QueryStorage<const C, Cs...> : QueryStorage<Cs...> {
	const Storage<const C> *storage = nullptr;

public:
	QueryStorage(World *p_world) :
			QueryStorage<Cs...>(p_world),
			storage(std::as_const(p_world)->get_storage<const C>()) {
	}

	EntitiesBuffer get_entities() const {
		// This is a determinant filter, that iterates over the existing
		// components of this storage.
		const EntitiesBuffer o_entities = QueryStorage<Cs...>::get_entities();
		if (unlikely(storage == nullptr)) {
			return o_entities;
		}
		const EntitiesBuffer entities = storage->get_stored_entities();
		return entities.count < o_entities.count ? entities : o_entities;
	}

	bool has_data(EntityID p_entity) const {
		if (unlikely(storage == nullptr)) {
			// This is a required field, since there is no storage this can end
			// immediately.
			return false;
		}
		return storage->has(p_entity) && QueryStorage<Cs...>::has_data(p_entity);
	}

	std::tuple<Batch<const C>, Batch<remove_filter_t<Cs>>...> get(EntityID p_id, Space p_mode) const {
#ifdef DEBUG_ENABLED
		// This can't happen because `is_done` returns true.
		CRASH_COND_MSG(storage == nullptr, "The storage" + String(typeid(Storage<C>).name()) + " is null.");
#endif

		return std::tuple_cat(
				std::tuple<Batch<const C>>(storage->get(p_id, p_mode)),
				QueryStorage<Cs...>::get(p_id, p_mode));
	}

	static void get_components(SystemExeInfo &r_info) {
		r_info.immutable_components.insert(C::get_component_id());
		QueryStorage<Cs...>::get_components(r_info);
	}
};

/// This is the fastest `Query`.
/// Using the variadic template, it's build at compile time. Since the
/// components must be known at compile time, this query can't by used by
/// scripts that have to rely on the `DynamicQuery`.
template <class... Cs>
class Query {
	/// Fetch space.
	Space m_space = LOCAL;

	/// List of entities to check.
	EntitiesBuffer entities = EntitiesBuffer(0, nullptr);

	// Storages
	QueryStorage<Cs...> q;

public:
	Query(World *p_world) :
			q(p_world) {
		// Prepare the query:
		// Ask all the pointed storage to return a list of entities to iterate;
		// the query, takes the smallest one, and iterates over it.
		entities = q.get_entities();
		if (unlikely(entities.count == UINT32_MAX)) {
			entities.count = 0;
			ERR_PRINT("This query is not valid, you are using only non determinant fileters (like `Without` and `Maybe`).");
		}
	}

	struct Iterator {
		using iterator_category = std::forward_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = std::tuple<Batch<remove_filter_t<Cs>>...>;

		Iterator(Query<Cs...> *p_query, const EntityID *p_entity) :
				query(p_query), entity(p_entity) {}

		bool is_valid() const {
			return *this != query->end();
		}

		value_type operator*() const {
			return query->q.get(*entity, query->m_space);
		}

		Iterator &operator++() {
			entity = query->next_valid_entity(entity);
			return *this;
		}

		Iterator operator++(int) {
			Iterator tmp = *this;
			++(*this);
			return tmp;
		}

		friend bool operator==(const Iterator &a, const Iterator &b) { return a.entity == b.entity; }
		friend bool operator!=(const Iterator &a, const Iterator &b) { return a.entity != b.entity; }

	private:
		Query<Cs...> *query;
		const EntityID *entity;
	};

	/// Allow to specify the space you want to fetch the data, you can use this
	/// in conjuction with the iterator and the random access:
	/// ```
	/// // Iterator example:
	/// for (auto [tr] : query.space(GLOBAL)) {
	/// 	// ...
	/// }
	///
	/// // Random access
	/// if (query.has(1)) {
	/// 	auto [tr] = query.space(GLOBAL)[1];
	/// }
	/// ```
	Query<Cs...> &space(Space p_space) {
		m_space = p_space;
		return *this;
	}

	/// Returns the forward `Iterator`, you can use in this way:
	/// ```
	/// for (auto [tr] : query) {
	/// 	// ...
	/// }
	/// ```
	Iterator begin() {
		// Returns the next available Entity.
		if (entities.count > 0) {
			if (q.has_data(*entities.entities) == false) {
				return Iterator(this, next_valid_entity(entities.entities));
			}
		}
		return Iterator(this, entities.entities);
	}

	/// Used to know the last element of the `Iterator`.
	Iterator end() {
		return Iterator(this, entities.entities + entities.count);
	}

	/// Returns true if this `Entity` exists and can be fetched.
	/// ```
	/// if(query.has(1)) {
	/// 	auto [component1, component2] = query[1];
	/// }
	/// ```
	bool has(EntityID p_entity) const {
		return q.has_data(p_entity);
	}

	/// Fetch the specific `Entity` component.
	/// If the `Entity` doesn't meet the query requirements, this function
	/// crashes, so it's necessary to use the function `has` to check if the
	///  entity can be fetched.
	/// ```
	/// if(query.has(1)) {
	/// 	auto [component1, component2] = query[1];
	/// }
	/// ```
	std::tuple<Batch<remove_filter_t<Cs>>...> operator[](EntityID p_entity) {
		return q.get(p_entity, m_space);
	}

	/// Counts the Entities that meets the requirements of this `Query`.
	/// IMPORTANT: Don't use this function to create C like loop: instead rely
	/// on the iterator.
	uint32_t count() {
		uint32_t count = 0;
		for (Iterator it = begin(); it != end(); ++it) {
			count += 1;
		}
		return count;
	}

	static void get_components(SystemExeInfo &r_info) {
		QueryStorage<Cs...>::get_components(r_info);
	}

private:
	const EntityID *next_valid_entity(const EntityID *p_current) {
		const EntityID *next = p_current + 1;

		// Search the next valid entity.
		for (; next != (entities.entities + entities.count); next += 1) {
			if (q.has_data(*next)) {
				// This is fine to return.
				return next;
			}
		}

		// Nothing more to iterate.
		return next;
	}
};
