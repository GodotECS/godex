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

/// Some storages have the ability to store multiple components per `Entity`,
/// to get all the stored components you can use the `Batch` filter:
/// `Query<Batch<MyEvent>> query;`
///
/// This filter allow nesting, indeed it's possible to specify sub filters:
/// - `Query<Batch<Changed<MyEvent>> query;`
/// - `Query<Batch<Maybe<const MyEvent>> query;`
/// - `Query<Batch<const MyEvent> query;`
///
/// You can access the data like an array, here an example:
/// ```
/// Query<Batch<Changed<MyEvent>> query;
/// for( auto [event] : query ){
/// 	if(event.is_empty()){
/// 		print_line("No events");
/// 	}else{
/// 		for(uint32_t i = 0; i < event.get_size(); i+=1){
/// 			print_line(event[i]);
/// 		}
/// 	}
/// }
/// ```
template <class C>
class Batch {
	C data;
	uint32_t size;

public:
	Batch(C p_data, uint32_t p_size) :
			data(p_data), size(p_size) {}
	Batch(const Batch &) = default;

	// Get the component at position `index`.
	C operator[](uint32_t p_index) {
#ifdef DEBUG_ENABLED
		CRASH_COND(p_index >= size);
#endif
		return data + p_index;
	}

	// Get the component at position `index`.
	const std::remove_const<C> operator[](uint32_t p_index) const {
#ifdef DEBUG_ENABLED
		CRASH_COND(p_index >= size);
#endif
		return data + p_index;
	}

	/// Get the components count.
	uint32_t get_size() const {
		return size;
	}

	/// Returns `true` if this contains 0 components.
	bool is_empty() const {
		return data == nullptr;
	}
};

/// With the `Flatten` filter you can specify many components, and it returns
/// the first valid. The `Flatten` filter, fetches the data if at least one of
/// its own filters is satisfied.
///
/// For example, this `Query<Flatten<TagA, TagB>>` returns all the entities
/// that contains TagA or TagB.
/// The syntax to extract the data is the following:
/// ```
/// Query<Flatten<TagA, TagB>> query;
/// auto [tag] = query[entity_1];
/// if( tag.is<TagA>() ){
/// 	TagA* tag_a = tag.as<TagA>();
/// } else
/// if( tag.is<TagB>() ){
/// 	TagB* tag_b = tag.as<TagB>();
/// }
/// ```
///
/// Note:
/// The `Flatten` filter, supports nesting. For example, you can use
/// the `Changed` filter in this way:
/// `Query<Flatten<const TagA, Changed<const TagB>>> query;`
/// Remember that the fist valid filter is returned.
/// The mutability is also important.
///
/// Known limitations:
/// `Query<Flatten<TagA, TagB>>` if you have an `Entity` that satisfy more
/// filters, like in the below case (**Entity 2**):
/// 	[Entity 0, TagA, ___]
/// 	[Entity 1, ___, TagB]
/// 	[Entity 2, TagA,TagB]
/// the query fetches the **Entity 2** twice, but the first specified component
/// is always taken (in this case the `TagA`).
/// _Remove this limitation would be a lot more expensinve than useful._
template <class... Cs>
struct Flatten {};

struct Flattened {
	void *const ptr;
	const godex::component_id id;
	const bool is_const;

	Flattened(void *p_ptr, godex::component_id p_id, bool p_const) :
			ptr(p_ptr), id(p_id), is_const(p_const) {}

	/// Returns `true` when the wrapped ptr is `nullptr`.
	bool is_null() const {
		return ptr == nullptr;
	}

	/// Returns `true` if `T` is a valid conversion. This function take into
	/// account mutability.
	/// ```
	/// Flattened flat;
	/// if( flat.is<TestComponent>() ){
	/// 	flat.as<TestComponent>();
	/// } else
	/// if ( flat.is<const TestComponent>() ) {
	/// 	flat.as<const TestComponent>();
	/// }
	/// ```
	template <class T>
	bool is() const {
		return id == T::get_component_id() &&
			   std::is_const<T>::value == is_const;
	}

	/// Unwrap the pointer, and cast it to T.
	/// It's possible to check the type using `is<TypeHere>()`.
	template <class T>
	T *as() {
#ifdef DEBUG_ENABLED
		// Just check the mutability here, no need to check the type also, so
		// it's possible to cast it easily to other types (like the base type).
		CRASH_COND_MSG(std::is_const<T>::value != is_const, "Please retrieve this data with the correct mutability.");
#endif
		return static_cast<T *>(ptr);
	}

	/// If the data is const, never return the pointer as non const.
	template <class T>
	const T *as() const {
		return static_cast<const std::remove_const_t<T> *>(ptr);
	}
};

// ~~ Utility to remove the filter from the query. ~~
template <typename T>
struct to_query_return_type {
	typedef T *type;
};

template <typename T>
struct to_query_return_type<Without<T>> {
	typedef T *type;
};

template <typename T>
struct to_query_return_type<Maybe<T>> {
	typedef T *type;
};

template <typename T>
struct to_query_return_type<Changed<T>> {
	typedef T *type;
};

template <>
struct to_query_return_type<EntityID> {
	typedef EntityID type;
};

template <typename... Ts>
struct to_query_return_type<Flatten<Ts...>> {
	typedef Flattened type;
};

template <typename T>
using to_query_return_type_t = typename to_query_return_type<T>::type;

template <typename T>
struct to_query_return_type<Batch<T>> {
	typedef Batch<to_query_return_type_t<T>> type;
};

/// `QueryStorage` specialization with 0 template arguments.
template <class... Cs>
struct QueryStorage {
	QueryStorage(World *p_world) {}

	EntitiesBuffer get_entities() const {
		// This is a NON determinant filter, so just return UINT32_MAX.
		return { UINT32_MAX, nullptr };
	}

	bool filter_satisfied(EntityID p_entity) const { return true; }
	std::tuple<to_query_return_type_t<Cs>...> get(EntityID p_id, Space p_mode) const { return std::tuple(); }

	static void get_components(SystemExeInfo &r_info) {}
};

// -------------------------------------------------------------------- EntityID

/// Fetch the `EntityID`.
template <class... Cs>
struct QueryStorage<EntityID, Cs...> : QueryStorage<Cs...> {
	QueryStorage(World *p_world) :
			QueryStorage<Cs...>(p_world) {}

	EntitiesBuffer get_entities() const {
		return QueryStorage<Cs...>::get_entities();
	}

	bool filter_satisfied(EntityID p_entity) const {
		return QueryStorage<Cs...>::filter_satisfied(p_entity);
	}

	std::tuple<EntityID, to_query_return_type_t<Cs>...> get(EntityID p_id, Space p_mode) {
		return std::tuple_cat(
				std::tuple<EntityID>(p_id),
				QueryStorage<Cs...>::get(p_id, p_mode));
	}

	static void get_components(SystemExeInfo &r_info) {
		QueryStorage<Cs...>::get_components(r_info);
	}
};

// ----------------------------------------------------------------------- Maybe

/// `QueryStorage` `Maybe` filter specialization.
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

	bool filter_satisfied(EntityID p_entity) const {
		// The `Maybe` filter never stops the execution.
		return QueryStorage<Cs...>::filter_satisfied(p_entity);
	}

	std::tuple<C *, to_query_return_type_t<Cs>...> get(EntityID p_id, Space p_mode) const {
		if (likely(storage != nullptr) && storage->has(p_id)) {
			if constexpr (std::is_const<C>::value) {
				return std::tuple_cat(
						std::tuple<C *>(const_cast<const Storage<C> *>(storage)->get(p_id, p_mode)),
						QueryStorage<Cs...>::get(p_id, p_mode));
			} else {
				return std::tuple_cat(
						std::tuple<C *>(storage->get(p_id, p_mode)),
						QueryStorage<Cs...>::get(p_id, p_mode));
			}
		} else {
			// Nothing to fetch, just set nullptr.
			return std::tuple_cat(
					std::tuple<C *>(nullptr),
					QueryStorage<Cs...>::get(p_id, p_mode));
		}
	}

	static void get_components(SystemExeInfo &r_info) {
		if constexpr (std::is_const<C>::value) {
			r_info.immutable_components.insert(C::get_component_id());
		} else {
			r_info.mutable_components.insert(C::get_component_id());
		}
		QueryStorage<Cs...>::get_components(r_info);
	}
};

// --------------------------------------------------------------------- Without

/// `QueryStorage` `Without` filter specialization.
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

	bool filter_satisfied(EntityID p_entity) const {
		if (unlikely(storage == nullptr)) {
			// When the storage is null the `Without` is always `true` though
			// we have to keep check the other storages.
			return QueryStorage<Cs...>::filter_satisfied(p_entity);
		}
		return storage->has(p_entity) == false && QueryStorage<Cs...>::filter_satisfied(p_entity);
	}

	std::tuple<C *, to_query_return_type_t<Cs>...> get(EntityID p_id, Space p_mode) const {
		// Just keep going, the `Without` filter doesn't collect data.
		return std::tuple_cat(std::tuple<C *>(nullptr), QueryStorage<Cs...>::get(p_id, p_mode));
	}

	static void get_components(SystemExeInfo &r_info) {
		// The `Without` collects the data always immutable.
		r_info.immutable_components.insert(C::get_component_id());
		QueryStorage<Cs...>::get_components(r_info);
	}
};

// --------------------------------------------------------------------- Changed

/// `QueryStorage` `Changed` filter specialization.
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

	bool filter_satisfied(EntityID p_entity) const {
		if (unlikely(storage == nullptr)) {
			// This is a required field, since there is no storage this can end
			// immediately.
			return false;
		}
		return storage->is_changed(p_entity) && QueryStorage<Cs...>::filter_satisfied(p_entity);
	}

	std::tuple<C *, to_query_return_type_t<Cs>...> get(EntityID p_id, Space p_mode) const {
#ifdef DEBUG_ENABLED
		// This can't happen because `is_done` returns true.
		CRASH_COND_MSG(storage == nullptr, "The storage" + String(typeid(Storage<C>).name()) + " is null.");
#endif

		if constexpr (std::is_const<C>::value) {
			return std::tuple_cat(
					std::tuple<C *>(const_cast<const Storage<C> *>(storage)->get(p_id, p_mode)),
					QueryStorage<Cs...>::get(p_id, p_mode));
		} else {
			return std::tuple_cat(
					std::tuple<C *>(storage->get(p_id, p_mode)),
					QueryStorage<Cs...>::get(p_id, p_mode));
		}
	}

	static void get_components(SystemExeInfo &r_info) {
		if constexpr (std::is_const<C>::value) {
			r_info.immutable_components.insert(C::get_component_id());
		} else {
			r_info.mutable_components.insert(C::get_component_id());
		}
		r_info.need_changed.insert(C::get_component_id());
		QueryStorage<Cs...>::get_components(r_info);
	}
};

// ----------------------------------------------------------------------- Batch

/// `QueryStorage` `Batch` filter specialization.
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

	bool filter_satisfied(EntityID p_entity) const {
		return query_storage.filter_satisfied(p_entity);
	}

	std::tuple<Batch<to_query_return_type_t<C>>, to_query_return_type_t<Cs>...> get(EntityID p_id, Space p_mode) const {
		Batch<to_query_return_type_t<C>> ret(nullptr, 0);
		if (query_storage.storage != nullptr) {
			auto [d] = query_storage.get(p_id, p_mode);
			ret = Batch(
					d,
					query_storage.storage->get_batch_size(p_id));
		}
		return std::tuple_cat(
				std::tuple<Batch<to_query_return_type_t<C>>>(ret),
				QueryStorage<Cs...>::get(p_id, p_mode));
	}

	static void get_components(SystemExeInfo &r_info) {
		QueryStorage<C>::get_components(r_info);
		QueryStorage<Cs...>::get_components(r_info);
	}
};

// --------------------------------------------------------------------- Flatten

template <class... Cs>
struct FlattenStorage {
	FlattenStorage(World *p_world) {}

	void get_entities(EntitiesBuffer r_buffers[], uint32_t p_index = 0) const {}
	bool filter_satisfied(EntityID p_entity) const { return false; }
	Flattened get(EntityID p_id, Space p_mode) const { return Flattened(nullptr, godex::COMPONENT_NONE, true); }
};

template <class C, class... Cs>
struct FlattenStorage<C, Cs...> : FlattenStorage<Cs...> {
	QueryStorage<C> storage;

	FlattenStorage(World *p_world) :
			FlattenStorage<Cs...>(p_world),
			storage(p_world) {
	}

	void get_entities(EntitiesBuffer r_buffers[], uint32_t p_index = 0) const {
		r_buffers[p_index] = storage.get_entities();
		FlattenStorage<Cs...>::get_entities(r_buffers, p_index + 1);
	}

	bool filter_satisfied(EntityID p_entity) const {
		if (storage.filter_satisfied(p_entity)) {
			// Found in storage, stop here.
			return true;
		} else {
			// Not in storage, try the next one.
			return FlattenStorage<Cs...>::filter_satisfied(p_entity);
		}
	}

	Flattened get(EntityID p_id, Space p_mode) const {
		if (storage.filter_satisfied(p_id)) {
			auto [d] = storage.get(p_id, p_mode);
			if constexpr (std::is_const<std::remove_pointer_t<to_query_return_type_t<C>>>::value) {
				return Flattened(
						const_cast<void *>(static_cast<const void *>(d)),
						std::remove_pointer_t<to_query_return_type_t<C>>::get_component_id(),
						true);
			} else {
				return Flattened(
						static_cast<void *>(d),
						std::remove_pointer_t<to_query_return_type_t<C>>::get_component_id(),
						false);
			}
		} else {
			return FlattenStorage<Cs...>::get(p_id, p_mode);
		}
	}
};

/// `QueryStorage` `Flatten` immutable filter specialization.
template <class... C, class... Cs>
struct QueryStorage<Flatten<C...>, Cs...> : QueryStorage<Cs...> {
	constexpr static std::size_t storages_count = sizeof...(C);
	FlattenStorage<C...> flat_storages;

	EntityID *entities_data = nullptr;
	EntitiesBuffer entities_buffer;

	QueryStorage(World *p_world) :
			QueryStorage<Cs...>(p_world),
			flat_storages(p_world) {
		// Fetch the entities from the storages, and creates a contiguous
		// memory that the query can fetch.
		EntitiesBuffer buffers[storages_count];
		flat_storages.get_entities(buffers);

		uint32_t sum = 0;
		for (uint32_t i = 0; i < storages_count; i += 1) {
			if (buffers[i].count == UINT32_MAX) {
				// One of the used sub filters is not determinant
				// (like `Without` and `Maybe`), so this filter can't drive
				// the iteration.
				// Just return `UINT32_MAX`.
				sum = UINT32_MAX;
				break;
			} else {
				sum += buffers[i].count;
			}
		}

		if (sum > 0 && sum < UINT32_MAX) {
			// This filter can drive the iteration.
			// Put all the `Entities` in one single array.

			// TODO Find a way to not allocate this memory!
			entities_data = (EntityID *)memalloc(sizeof(EntityID) * sum);

			// TODO Find a way to not copy this memory!
			for (uint32_t i = 0, offset = 0; i < storages_count; i += 1) {
				memcpy(entities_data + offset, buffers[i].entities, sizeof(EntityID) * buffers[i].count);
				offset += buffers[i].count;
			}

			// TODO or at least, find a way to not do this each frame.
		}

		entities_buffer.count = sum;
		entities_buffer.entities = entities_data;
	}

	~QueryStorage() {
		if (entities_data != nullptr) {
			memfree(entities_data);
		}
	}

	EntitiesBuffer get_entities() const {
		return entities_buffer;
	}

	bool filter_satisfied(EntityID p_entity) const {
		return flat_storages.filter_satisfied(p_entity);
	}

	std::tuple<Flattened, to_query_return_type_t<Cs>...> get(EntityID p_id, Space p_mode) const {
		return std::tuple_cat(
				std::tuple<Flattened>(flat_storages.get(p_id, p_mode)),
				QueryStorage<Cs...>::get(p_id, p_mode));
	}

	static void get_components(SystemExeInfo &r_info) {
		// Take the components storages used, taking advantage of the
		// get_components function.
		QueryStorage<C...>::get_components(r_info);
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

	bool filter_satisfied(EntityID p_entity) const {
		if (unlikely(storage == nullptr)) {
			// This is a required field, since there is no storage this can end
			// immediately.
			return false;
		}
		return storage->has(p_entity) && QueryStorage<Cs...>::filter_satisfied(p_entity);
	}

	std::tuple<C *, to_query_return_type_t<Cs>...> get(EntityID p_id, Space p_mode) const {
#ifdef DEBUG_ENABLED
		// This can't happen because `is_done` returns true.
		CRASH_COND_MSG(storage == nullptr, "The storage" + String(typeid(Storage<C>).name()) + " is null.");
#endif

		if constexpr (std::is_const<C>::value) {
			return std::tuple_cat(
					std::tuple<C *>(const_cast<const Storage<C> *>(storage)->get(p_id, p_mode)),
					QueryStorage<Cs...>::get(p_id, p_mode));
		} else {
			return std::tuple_cat(
					std::tuple<C *>(storage->get(p_id, p_mode)),
					QueryStorage<Cs...>::get(p_id, p_mode));
		}
	}

	static void get_components(SystemExeInfo &r_info) {
		if constexpr (std::is_const<C>::value) {
			r_info.immutable_components.insert(C::get_component_id());
		} else {
			r_info.mutable_components.insert(C::get_component_id());
		}
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
		using value_type = std::tuple<to_query_return_type_t<Cs>...>;

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
			if (q.filter_satisfied(*entities.entities) == false) {
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
		return q.filter_satisfied(p_entity);
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
	std::tuple<to_query_return_type_t<Cs>...> operator[](EntityID p_entity) {
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
			if (q.filter_satisfied(*next)) {
				// This is fine to return.
				return next;
			}
		}

		// Nothing more to iterate.
		return next;
	}
};
