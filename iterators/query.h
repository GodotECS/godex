/** @author AndreaCatania */

#pragma once

#include "../storage/storage.h"
#include "../systems/system.h"
#include "../world/world.h"
#include <tuple>

// ~~ Query filters. ~~
template <class C>
struct Without {};

template <class C>
struct Maybe {};

template <class C>
struct Changed {};

/// `Batch` it's used by the queries to return multiple `Component`s for
/// entity. Depending on the storage used, it's possible to store more
/// components per entity; in all these cases the `Batch` filter can be used
/// to retrieve those.
template <class C>
class Batch {
	C data;
	uint32_t size;

public:
	Batch(C p_data, uint32_t p_size) :
			data(p_data), size(p_size) {}
	Batch(const Batch &) = default;

	C operator[](uint32_t p_index) {
#ifdef DEBUG_ENABLED
		CRASH_COND(p_index >= size);
#endif
		return data + p_index;
	}

	C operator[](uint32_t p_index) const {
#ifdef DEBUG_ENABLED
		CRASH_COND(p_index >= size);
#endif
		return data + p_index;
	}

	uint32_t get_size() const {
		return size;
	}

	bool is_empty() const {
		return data == nullptr;
	}
};

/// Return a pointer that can be of a specific type.
template <class... Cs>
struct PickValid {
private:
	void *ptr;
	godex::component_id id;

public:
	PickValid(void *p_ptr, godex::component_id p_id) :
			ptr(p_ptr), id(p_id) {}

	template <class T>
	bool is() const {
		return id == T::get_component_id();
	}

	template <class T>
	T *get() {
		return static_cast<T *>(ptr);
	}

	template <class T>
	const T *get() const {
		return static_cast<const T *>(ptr);
	}
};

// ~~ Utility to remove the filter from the query. ~~
template <typename T>
struct remove_filter {
	typedef T *type;
};

template <typename T>
struct remove_filter<Without<T>> {
	typedef T *type;
};

template <typename T>
struct remove_filter<Maybe<T>> {
	typedef T *type;
};

template <typename T>
struct remove_filter<Changed<T>> {
	typedef T *type;
};

template <>
struct remove_filter<EntityID> {
	typedef EntityID type;
};

template <typename T>
using remove_filter_t = typename remove_filter<T>::type;

template <typename T>
struct remove_filter<Batch<T>> {
	typedef Batch<remove_filter_t<T>> type;
};

template <typename T>
struct remove_filter<const Batch<T>> {
	typedef const Batch<remove_filter_t<const T>> type;
};

template <typename T>
struct remove_filter<PickValid<T>> {
	typedef PickValid<T> type;
};

template <typename T>
struct remove_filter<const PickValid<T>> {
	typedef const PickValid<T> type;
};

// ~~ Query storage, is used to fetch a single data. ~~

/// `QueryStorage` specialization with 0 template arguments.
template <class... Cs>
struct QueryStorage {
	QueryStorage(World *p_world) {}

	EntitiesBuffer get_entities() const {
		// This is a NON determinant filter, so just return UINT32_MAX.
		return { UINT32_MAX, nullptr };
	}

	bool has_data(EntityID p_entity) const { return true; }
	std::tuple<remove_filter_t<Cs>...> get(EntityID p_id, Space p_mode) const { return std::tuple(); }

	static void get_components(SystemExeInfo &r_info) {}
};

/// Fetch the `EntityID`.
template <class... Cs>
struct QueryStorage<EntityID, Cs...> : QueryStorage<Cs...> {
	QueryStorage(World *p_world) :
			QueryStorage<Cs...>(p_world) {}

	EntitiesBuffer get_entities() const {
		return QueryStorage<Cs...>::get_entities();
	}

	bool has_data(EntityID p_entity) const {
		return QueryStorage<Cs...>::has_data(p_entity);
	}

	std::tuple<EntityID, remove_filter_t<Cs>...> get(EntityID p_id, Space p_mode) {
		return std::tuple_cat(
				std::tuple<EntityID>(p_id),
				QueryStorage<Cs...>::get(p_id, p_mode));
	}

	static void get_components(SystemExeInfo &r_info) {
		QueryStorage<Cs...>::get_components(r_info);
	}
};

/// `QueryStorage` `Maybe` mutable filter specialization.
template <class C, class... Cs>
struct QueryStorage<Maybe<C>, Cs...> : QueryStorage<Cs...> {
	Storage<C> *storage = nullptr;

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

	std::tuple<C *, remove_filter_t<Cs>...> get(EntityID p_id, Space p_mode) const {
		if (likely(storage != nullptr) && storage->has(p_id)) {
			return std::tuple_cat(
					std::tuple<C *>(storage->get(p_id, p_mode)),
					QueryStorage<Cs...>::get(p_id, p_mode));
		} else {
			// Nothing to fetch, just set nullptr.
			return std::tuple_cat(
					std::tuple<C *>(nullptr),
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
struct QueryStorage<Maybe<const C>, Cs...> : QueryStorage<Cs...> {
	const Storage<const C> *storage = nullptr;

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

	std::tuple<const C *, remove_filter_t<Cs>...> get(EntityID p_id, Space p_mode) const {
		if (likely(storage != nullptr) && storage->has(p_id)) {
			return std::tuple_cat(
					std::tuple<const C *>(storage->get(p_id, p_mode)),
					QueryStorage<Cs...>::get(p_id, p_mode));
		} else {
			// Nothing to fetch, just set nullptr.
			return std::tuple_cat(
					std::tuple<const C *>(nullptr),
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
struct QueryStorage<Without<C>, Cs...> : QueryStorage<Cs...> {
	Storage<C> *storage = nullptr;

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

	std::tuple<C *, remove_filter_t<Cs>...> get(EntityID p_id, Space p_mode) const {
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
struct QueryStorage<Changed<C>, Cs...> : QueryStorage<Cs...> {
	Storage<C> *storage = nullptr;

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

	std::tuple<C *, remove_filter_t<Cs>...> get(EntityID p_id, Space p_mode) const {
#ifdef DEBUG_ENABLED
		// This can't happen because `is_done` returns true.
		CRASH_COND_MSG(storage == nullptr, "The storage" + String(typeid(Storage<C>).name()) + " is null.");
#endif

		return std::tuple_cat(
				std::tuple<C *>(storage->get(p_id, p_mode)),
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
struct QueryStorage<Changed<const C>, Cs...> : QueryStorage<Cs...> {
	const Storage<const C> *storage = nullptr;

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

	std::tuple<const C *, remove_filter_t<Cs>...> get(EntityID p_id, Space p_mode) const {
#ifdef DEBUG_ENABLED
		// This can't happen because `is_done` returns true.
		CRASH_COND_MSG(storage == nullptr, "The storage" + String(typeid(Storage<C>).name()) + " is null.");
#endif

		return std::tuple_cat(
				std::tuple<const C *>(storage->get(p_id, p_mode)),
				QueryStorage<Cs...>::get(p_id, p_mode));
	}

	static void get_components(SystemExeInfo &r_info) {
		r_info.immutable_components.insert(C::get_component_id());
		r_info.need_changed.insert(C::get_component_id());
		QueryStorage<Cs...>::get_components(r_info);
	}
};

template <class... Cs>
struct PickValidStorage {
	PickValidStorage(World *p_world) {}

	EntitiesBuffer get_entities() const { return EntitiesBuffer(0, nullptr); }
	bool has_data(EntityID p_entity) const { return false; }
	std::tuple<PickValid<remove_filter_t<Cs>>...> get(EntityID p_id, Space p_mode) const { return PickValid(nullptr, godex::COMPONENT_NONE); }
};

template <class C, class... Cs>
struct PickValidStorage<const C, Cs...> : PickValidStorage<Cs...> {
	QueryStorage<C> storage;

	PickValidStorage(World *p_world) :
			PickValidStorage<Cs...>(p_world),
			storage(p_world) {
	}

	EntitiesBuffer get_entities() const {
		const EntitiesBuffer o_entities = PickValidStorage<Cs...>::get_entities();
		const EntitiesBuffer entities = storage.get_entities();
		return entities.count < o_entities.count ? entities : o_entities;
	}

	bool has_data(EntityID p_entity) const {
		if (storage != nullptr && storage.has_data(p_entity)) {
			// Found in storage.
			return true;
		} else {
			// Not in storage, try the next one.
			return PickValidStorage<Cs...>::has_data(p_entity);
		}
	}

	std::tuple<C *, remove_filter_t<Cs>...> get(EntityID p_id, Space p_mode) const {
		if (has_data(p_id)) {
			return PickValid(storage.get(p_id, p_mode), C::get_component_id());
		} else {
			return PickValidStorage<Cs...>::get(p_id, p_mode);
		}
	}
};

/// `QueryStorage` `PickValid` immutable filter specialization.
//template <class... C, class... Cs>
//struct QueryStorage<const PickValid<C...>, Cs...> : QueryStorage<Cs...> {
//	PickValidStorage<const C...> sub_storages;
//
//	QueryStorage(World *p_world) :
//			QueryStorage<Cs...>(p_world),
//			sub_storages(p_world) {
//	}
//
//	EntitiesBuffer get_entities() const {
//		return sub_storages.get_entities();
//	}
//
//	bool has_data(EntityID p_entity) const {
//		return sub_storages.has_data(p_entity);
//	}
//
//	std::tuple<Batch<const PickValid<C...>>, Batch<remove_filter_t<Cs>>...> get(EntityID p_id, Space p_mode) const {
//		return sub_storages.get(p_id, p_mode);
//	}
//
//	static void get_components(SystemExeInfo &r_info) {
//		// Take the components storages used, taking advantage of the
//		// get_components function.
//		QueryStorage<const C...>::get_components(r_info);
//		QueryStorage<Cs...>::get_components(r_info);
//	}
//};

template <class C, class... Cs>
struct QueryStorage<Batch<C>, Cs...> : QueryStorage<Cs...> {
	QueryStorage<C> query_storage;

	QueryStorage(World *p_world) :
			QueryStorage<Cs...>(p_world),
			query_storage(p_world) {
	}

	EntitiesBuffer get_entities() const {
		return query_storage.get_entities();
	}

	bool has_data(EntityID p_entity) const {
		return query_storage.has_data(p_entity);
	}

	std::tuple<Batch<remove_filter_t<C>>, remove_filter_t<Cs>...> get(EntityID p_id, Space p_mode) const {
		Batch<remove_filter_t<C>> ret(nullptr, 0);
		if (query_storage.storage != nullptr) {
			auto [d] = query_storage.get(p_id, p_mode);
			ret = Batch(
					d,
					query_storage.storage->get_batch_size(p_id));
		}
		return std::tuple_cat(
				std::tuple<Batch<remove_filter_t<C>>>(ret),
				QueryStorage<Cs...>::get(p_id, p_mode));
	}

	static void get_components(SystemExeInfo &r_info) {
		QueryStorage<C>::get_components(r_info);
		QueryStorage<Cs...>::get_components(r_info);
	}
};

/// `QueryStorage` no filter specialization.
template <class C, class... Cs>
struct QueryStorage<C, Cs...> : QueryStorage<Cs...> {
	Storage<C> *storage = nullptr;

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

	std::tuple<C *, remove_filter_t<Cs>...> get(EntityID p_id, Space p_mode) const {
#ifdef DEBUG_ENABLED
		// This can't happen because `is_done` returns true.
		CRASH_COND_MSG(storage == nullptr, "The storage" + String(typeid(Storage<C>).name()) + " is null.");
#endif

		return std::tuple_cat(
				std::tuple<C *>(storage->get(p_id, p_mode)),
				QueryStorage<Cs...>::get(p_id, p_mode));
	}

	static void get_components(SystemExeInfo &r_info) {
		r_info.mutable_components.insert(C::get_component_id());
		QueryStorage<Cs...>::get_components(r_info);
	}
};

/// `QueryStorage` const no filter specialization.
template <class C, class... Cs>
struct QueryStorage<const C, Cs...> : QueryStorage<Cs...> {
	const Storage<const C> *storage = nullptr;

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

	std::tuple<const C *, remove_filter_t<Cs>...> get(EntityID p_id, Space p_mode) const {
#ifdef DEBUG_ENABLED
		// This can't happen because `is_done` returns true.
		CRASH_COND_MSG(storage == nullptr, "The storage" + String(typeid(Storage<C>).name()) + " is null.");
#endif

		return std::tuple_cat(
				std::tuple<const C *>(storage->get(p_id, p_mode)),
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
		using value_type = std::tuple<remove_filter_t<Cs>...>;

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
	std::tuple<remove_filter_t<Cs>...> operator[](EntityID p_entity) {
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
