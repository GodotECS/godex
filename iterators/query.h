#pragma once

#include "../storage/storage.h"
#include "../systems/system.h"
#include "../world/world.h"

// ---------------------------------------------------------------- Query filters

template <class C>
struct Not {};

template <class C>
struct Create {};

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
	Batch() = default;
	Batch(const Batch &) = default;
	Batch(C p_data, uint32_t p_size) :
			data(p_data), size(p_size) {}

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

/// The `Any` filter is satisfied if at least 1 sub filter is satisfied:
/// in such cases all are ruturned or `nullptr` instead.
///
/// ```
/// Query<Any<Changed<Transform>, Changed<Active>> query;
/// ```
///
/// NOTICE: You can't nest `Any` twice like this:
/// ```
/// Query<Any<Any<Transform, Active>> query;
/// ```
/// (It doesn't make sense)
template <class... Cs>
struct Any {};

/// The `Join` filter is just an utility that collapse the sub components
/// to one, taking the first `satisfied`.
///
/// For example, this `Query<Join<TagA, TagB>>` returns all the entities
/// that contains TagA or TagB.
/// The syntax to extract the data is the following:
/// ```
/// Query<Join<TagA, TagB>> query;
///
/// auto [tag] = query[entity_1];
/// if( tag.is<TagA>() ){
/// 	TagA* tag_a = tag.as<TagA>();
/// } else
/// if( tag.is<TagB>() ){
/// 	TagB* tag_b = tag.as<TagB>();
/// }
/// ```
template <class... Cs>
struct Join {};

struct JoinData {
private:
	void *ptr;
	godex::component_id id;
	bool is_const;

public:
	JoinData(void *p_ptr, godex::component_id p_id, bool p_const) :
			ptr(p_ptr), id(p_id), is_const(p_const) {}
	JoinData() = default;
	JoinData(const JoinData &) = default;

	/// Returns `true` when the wrapped ptr is `nullptr`.
	bool is_null() const {
		return ptr == nullptr;
	}

	/// Returns `true` if `T` is a valid conversion. This function take into
	/// account mutability.
	/// ```
	/// Join join;
	/// if( join.is<TestComponent>() ){
	/// 	join.as<TestComponent>();
	/// } else
	/// if ( join.is<const TestComponent>() ) {
	/// 	join.as<const TestComponent>();
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
		if (unlikely(ptr != nullptr)) {
			ERR_FAIL_COND_V_MSG(std::is_const<T>::value != is_const, nullptr, "Please retrieve this JOINED data with the correct mutability.");
		}
#endif
		return static_cast<T *>(ptr);
	}

	/// If the data is const, never return the pointer as non const.
	template <class T>
	const T *as() const {
		return static_cast<const std::remove_const_t<T> *>(ptr);
	}
};

// --------------------------------------------------------- Fetch Elements Type

/// This is an utility that is used by the `QueryResultTuple` to fetch the
/// variable type for a specific element of the query.
/// The first template parameter `S`, stands for `Search`, and can be used to
/// point a specific Query element.
/// The following templates are just the `Query` definition.
///
/// ```cpp
/// fetch_element_type<0, 0, int, Any<int, float>, bool> // This is int*
/// fetch_element_type<1, 0, int, Any<int, float>, bool> // This is float*
///
/// fetch_element_type<0, 0, Not<int>, Any<int, Batch<Changed<float>>>, Maybe<bool>, EntityID> // This is int*
/// fetch_element_type<1, 0, Not<int>, Any<int, Batch<Changed<float>>>, Maybe<bool>, EntityID> // This is int*
/// fetch_element_type<2, 0, Not<int>, Any<int, Batch<Changed<float>>>, Maybe<bool>, EntityID> // This is Batch<float*>
/// fetch_element_type<3, 0, Not<int>, Any<int, Batch<Changed<float>>>, Maybe<bool>, EntityID> // This is float*
/// fetch_element_type<4, 0, Not<int>, Any<int, Batch<Changed<float>>>, Maybe<bool>, EntityID> // This is bool*
/// ```
///
/// # How it works
/// When you call `fetch_element_type`, as shown, the compiler fetches the `type`
/// of the element where `S` and `I` are the same.
///
/// @tparam S stands for `Search`
/// @tparam I stands for `Index`, this must always be 0 when you define it.
/// @tparam ...Cs The Query type.
template <std::size_t S, std::size_t I, class... Cs>
struct fetch_element {};

/// Just an utility.
template <std::size_t S, std::size_t B, class... Cs>
using fetch_element_type = typename fetch_element<S, B, Cs...>::type;

/// Query element type found: It's just a pointer.
template <std::size_t S, class C, class... Cs>
struct fetch_element<S, S, C, Cs...> : fetch_element<S, S + 1, Cs...> {
	using type = C *;
};

/// We found the `Changed` filter, fetch the type now.
template <std::size_t S, class C, class... Cs>
struct fetch_element<S, S, Changed<C>, Cs...> : fetch_element<S, S + 1, Cs...> {
	// Keep search the sub type: so we can nest with other filters.
	using type = fetch_element_type<0, 0, C>;
};

/// We found the `Not` filter, fetch the type now.
template <std::size_t S, class C, class... Cs>
struct fetch_element<S, S, Not<C>, Cs...> : fetch_element<S, S + 1, Cs...> {
	// Keep search the sub type: so we can nest with other filters.
	using type = fetch_element_type<0, 0, C>;
};

/// We found the `Create` filter, fetch the type now.
template <std::size_t S, class C, class... Cs>
struct fetch_element<S, S, Create<C>, Cs...> : fetch_element<S, S + 1, Cs...> {
	// Just C *;
	using type = C *;
};

/// We found the `Maybe` filter, fetch the type now.
template <std::size_t S, class C, class... Cs>
struct fetch_element<S, S, Maybe<C>, Cs...> : fetch_element<S, S + 1, Cs...> {
	// Keep search the sub type: so we can nest with other filters.
	using type = fetch_element_type<0, 0, C>;
};

/// We found the `Batch` filter, fetch the type now.
template <std::size_t S, class C, class... Cs>
struct fetch_element<S, S, Batch<C>, Cs...> : fetch_element<S, S + 1, Cs...> {
	// Append `Batch` but also keep search the sub type: so we can nest other filters.
	using type = Batch<fetch_element_type<0, 0, C>>;
};

/// We found the `Join` filter, fetch the type now.
template <std::size_t S, class... C, class... Cs>
struct fetch_element<S, S, Join<C...>, Cs...> : fetch_element<S, S + 1, Cs...> {
	// Just JoinData.
	using type = JoinData;
};

/// We found the `EntityID` filter, fetch the type now.
template <std::size_t S, class... Cs>
struct fetch_element<S, S, EntityID, Cs...> : fetch_element<S, S + 1, Cs...> {
	// The type is just an `EntityID`.
	using type = EntityID;
};

/// Found specialization `Any` where `S` and `I` are the same: Flatten it as usual.
template <std::size_t S, class... C, class... Cs>
struct fetch_element<S, S, Any<C...>, Cs...> : fetch_element<S, S, C..., Cs...> {
};

/// Found specialization `Any`: Just flatten its sub types.
template <std::size_t S, std::size_t I, class... C, class... Cs>
struct fetch_element<S, I, Any<C...>, Cs...> : fetch_element<S, I, C..., Cs...> {};

/// `S` and `I` are not yet the same: Just check the next `I`.
template <std::size_t S, std::size_t I, class C, class... Cs>
struct fetch_element<S, I, C, Cs...> : fetch_element<S, I + 1, Cs...> {};

// ---------------------------------------------------------- Query Result Tuple

/// This class is the base query result tuple.
///
/// `I` stands for `Index` and it's used to index the types.
///
/// `I` must be always 0 when you define a new tuple so it can index all the
/// components correctly.
/// However, never use this directly rather use `QueryResultTuple`.
template <std::size_t I, class... Cs>
struct QueryResultTuple_Impl {
	/// The count of elements in this tuple.
	static constexpr std::size_t SIZE = I;
};

/// No filter specialization.
template <std::size_t I, class C, class... Cs>
struct QueryResultTuple_Impl<I, C, Cs...> : public QueryResultTuple_Impl<I + 1, Cs...> {
	C *value = nullptr;
};

/// `Batch` filter specialization.
template <std::size_t I, class C, class... Cs>
struct QueryResultTuple_Impl<I, Batch<C>, Cs...> : public QueryResultTuple_Impl<I + 1, Cs...> {
	Batch<fetch_element_type<0, 0, C>> value = Batch<fetch_element_type<0, 0, C>>(nullptr, 0);
};

/// `Join` filter specialization.
template <std::size_t I, class... C, class... Cs>
struct QueryResultTuple_Impl<I, Join<C...>, Cs...> : public QueryResultTuple_Impl<I + 1, Cs...> {
	JoinData value = JoinData(nullptr, godex::COMPONENT_NONE, true);
};

/// `EntityID` filter specialization.
template <std::size_t I, class... Cs>
struct QueryResultTuple_Impl<I, EntityID, Cs...> : public QueryResultTuple_Impl<I + 1, Cs...> {
	EntityID value;
};

/// Flatten all the Filter, so we can store the data on the same level.
/// This template is able to flatten the filters: `Changed`, `Not`, `Maybe`, `Any`, `Join`
///
/// Notice: This is just forwarding the declaration, indeed this has the same
/// index the flattened data has.
///
/// ```
/// This:
/// QeryResultTuple_Impl<0, int, Not<float>>
///
/// Is compiled as:
/// `ResultTuple<0, int> :
/// 		 ResultTuple<1, Not<>> :
/// 				ResultTuple<1, float>
/// ```
/// The `set` and `get` functions are able to fetch the data anyway.
template <std::size_t I, class... C, class... Cs, template <class> class Filter>
struct QueryResultTuple_Impl<I, Filter<C...>, Cs...> : public QueryResultTuple_Impl<I, C..., Cs...> {};

/// `QueryResultTuple` is the tuple returned by the `Query`. You can fetch the
/// data using the `get` function:
/// ```cpp
/// Query<Transform, Mesh> query;
/// QueryResult result = query.get();
/// Transform* t = get<0>(result);
/// ```
///
/// Or you can use the structured bindings:
/// ```cpp
/// Query<Transform, Mesh> query;
/// QueryResult result = query.get();
/// auto [transform, mesh] = result;
/// ```
template <class... Cs>
struct QueryResultTuple : public QueryResultTuple_Impl<0, Cs...> {};

// ---------------------------------------------------------- Structured Bindings

/// Bindings to allow use structured bidnings.
/// These are necessary to tell the compiler how to decompose the custom class.
namespace std {
template <class... Cs>
struct tuple_size<QueryResultTuple<Cs...>> : std::integral_constant<int, QueryResultTuple<Cs...>::SIZE> {};

template <size_t S, class... Cs>
struct tuple_element<S, QueryResultTuple<Cs...>> {
	using type = typename fetch_element<S, 0, Cs...>::type;
};
} // namespace std

// ----------------------------------------------------------------- Tuple Setter

/// Set the data inside the tuple at give index `S`.
template <std::size_t S, class T, class C, class... Cs>
constexpr void set_impl(T p_val, QueryResultTuple_Impl<S, C, Cs...> &tuple) noexcept {
	tuple.value = p_val;
}

/// Set the batched data inside the tuple at give index `S`.
template <std::size_t S, class T, class C, class... Cs>
constexpr void set_impl(T p_val, QueryResultTuple_Impl<S, Batch<C>, Cs...> &tuple) noexcept {
	tuple.value = p_val;
}

/// Set the joined data inside the tuple at give index `S`.
template <std::size_t S, class T, class... C, class... Cs>
constexpr void set_impl(T p_val, QueryResultTuple_Impl<S, Join<C...>, Cs...> &tuple) noexcept {
	tuple.value = p_val;
}

/**
 * NOTE: I tried to use this:
 * ```
 * template <std::size_t S, class T, class... C, class... Cs, template <class> class Filter>
 * constexpr void set_impl(T p_val, QueryResultTuple_Impl<S, Filter<C>, Cs...> &tuple) noexcept {}
 * ```
 * but it doesn't recursivelly calls itself when there are more than 1 nested
 * level. So I've separated these.
 */

/// Remove the filter `Changed` and set the value.
template <std::size_t S, class T, class C, class... Cs>
constexpr void set_impl(T p_val, QueryResultTuple_Impl<S, Changed<C>, Cs...> &tuple) noexcept {
	// Forward to subfilters.
	set_impl<S>(p_val, static_cast<QueryResultTuple_Impl<S, C, Cs...> &>(tuple));
}

/// Ignore the filter `Not` and set the value.
template <std::size_t S, class T, class C, class... Cs>
constexpr void set_impl(T p_val, QueryResultTuple_Impl<S, Not<C>, Cs...> &tuple) noexcept {
	// Forward to subfilters.
	set_impl<S>(p_val, static_cast<QueryResultTuple_Impl<S, C, Cs...> &>(tuple));
}

/// Ignore the filter `Create` and set the value.
template <std::size_t S, class T, class C, class... Cs>
constexpr void set_impl(T p_val, QueryResultTuple_Impl<S, Create<C>, Cs...> &tuple) noexcept {
	set_impl<S>(p_val, static_cast<QueryResultTuple_Impl<S, C, Cs...> &>(tuple));
}

/// Ignore the filter `Maybe` and set the value.
template <std::size_t S, class T, class C, class... Cs>
constexpr void set_impl(T p_val, QueryResultTuple_Impl<S, Maybe<C>, Cs...> &tuple) noexcept {
	// Forward to subfilters.
	set_impl<S>(p_val, static_cast<QueryResultTuple_Impl<S, C, Cs...> &>(tuple));
}

/// Ignore the filter `Any` and set the value.
template <std::size_t S, class T, class... C, class... Cs>
constexpr void set_impl(T p_val, QueryResultTuple_Impl<S, Any<C...>, Cs...> &tuple) noexcept {
	// Forward to subfilters.
	set_impl<S>(p_val, static_cast<QueryResultTuple_Impl<S, C..., Cs...> &>(tuple));
}

/// Can be used to set the data inside a tuple.
template <std::size_t S, class T, class... Cs>
constexpr void set(QueryResultTuple<Cs...> &tuple, T p_val) noexcept {
	set_impl<S>(p_val, tuple);
}

// ----------------------------------------------------------------- Tuple Getter

/// Fetches the data from the the tuple, because `Search` is equal to the element `Index`.
template <std::size_t S, class C, class... Cs>
constexpr auto get_impl(const QueryResultTuple_Impl<S, C, Cs...> &tuple) noexcept {
	return tuple.value;
}

/// Fetches the batched data from the tuple.
template <std::size_t S, class C, class... Cs>
constexpr auto get_impl(const QueryResultTuple_Impl<S, Batch<C>, Cs...> &tuple) noexcept {
	return tuple.value;
}

/// Fetches the joined data from the tuple.
template <std::size_t S, class... C, class... Cs>
constexpr auto get_impl(const QueryResultTuple_Impl<S, Join<C...>, Cs...> &tuple) noexcept {
	return tuple.value;
}

/// Fetches the Created data from the tuple.
template <std::size_t S, class C, class... Cs>
constexpr auto get_impl(const QueryResultTuple_Impl<S, Create<C>, Cs...> &tuple) noexcept {
	return tuple.value;
}

/**
 * NOTE: I tried to use this:
 * ```
 * template <std::size_t S, class... C, class... Cs, template <class...> class Filter>
 * constexpr auto get_impl(const QueryResultTuple_Impl<S, Filter<C...>, Cs...> &tuple) noexcept {
 * ```
 * but it doesn't recursivelly calls itself when there are more than 1 nested
 * level. So I've separated these.
 */

/// Skip the filter `Changed`.
template <std::size_t S, class C, class... Cs>
constexpr auto get_impl(const QueryResultTuple_Impl<S, Changed<C>, Cs...> &tuple) noexcept {
	// Forward to subfilters.
	return get_impl<S>(static_cast<const QueryResultTuple_Impl<S, C, Cs...> &>(tuple));
}

/// Skip the filter `Not`.
template <std::size_t S, class C, class... Cs>
constexpr auto get_impl(const QueryResultTuple_Impl<S, Not<C>, Cs...> &tuple) noexcept {
	// Forward to subfilters.
	return get_impl<S>(static_cast<const QueryResultTuple_Impl<S, C, Cs...> &>(tuple));
}

/// Skip the filter `Maybe`.
template <std::size_t S, class C, class... Cs>
constexpr auto get_impl(const QueryResultTuple_Impl<S, Maybe<C>, Cs...> &tuple) noexcept {
	// Forward to subfilters.
	return get_impl<S>(static_cast<const QueryResultTuple_Impl<S, C, Cs...> &>(tuple));
}

/// Skip the filter `Any`.
template <std::size_t S, class... C, class... Cs>
constexpr auto get_impl(const QueryResultTuple_Impl<S, Any<C...>, Cs...> &tuple) noexcept {
	// Forward to subfilters.
	return get_impl<S>(static_cast<const QueryResultTuple_Impl<S, C..., Cs...> &>(tuple));
}

/// Can be used to fetch the data from a tuple.
template <std::size_t S, class... Cs>
constexpr fetch_element_type<S, 0, Cs...> get(const QueryResultTuple<Cs...> &tuple) noexcept {
	return get_impl<S>(tuple);
}

// --------------------------------------------------------------- Query Storages

/// `QueryStorage` specialization with 0 template arguments.
template <std::size_t I, class... Cs>
struct QueryStorage {
	QueryStorage(World *p_world) {}
	void set_world_notification_active(bool p_active) {}
	void initiate_process(World *p_world) {}
	void conclude_process(World *p_world) {}

	constexpr static bool is_filter_derminant() {
		// False by default.
		return false;
	}
	EntitiesBuffer get_entities() const {
		// This is a NON determinant filter, so just return UINT32_MAX.
		return { UINT32_MAX, nullptr };
	}

	bool filter_satisfied(EntityID p_entity) const { return true; }
	bool can_fetch(EntityID p_entity) const { return false; }

	template <class... Qs>
	void fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
		// Nothing to fetch.
	}

	static void get_components(SystemExeInfo &r_info, const bool p_force_immutable = false) {}
};

// -------------------------------------------------------------------- EntityID

/// Fetch the `EntityID`.
template <std::size_t I, class... Cs>
struct QueryStorage<I, EntityID, Cs...> : public QueryStorage<I + 1, Cs...> {
	QueryStorage(World *p_world) :
			QueryStorage<I + 1, Cs...>(p_world) {}
	void initiate_process(World *p_world) {
		QueryStorage<I + 1, Cs...>::initiate_process(p_world);
	}
	void conclude_process(World *p_world) {
		QueryStorage<I + 1, Cs...>::conclude_process(p_world);
	}
	void set_world_notification_active(bool p_active) {
		QueryStorage<I + 1, Cs...>::set_world_notification_active(p_active);
	}

	EntitiesBuffer get_entities() const {
		return QueryStorage<I + 1, Cs...>::get_entities();
	}

	bool filter_satisfied(EntityID p_entity) const {
		return QueryStorage<I + 1, Cs...>::filter_satisfied(p_entity);
	}

	bool can_fetch(EntityID p_entity) const {
		return true;
	}

	template <class... Qs>
	void fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
		// Set the `EntityID` at position `I`.
		set<I>(r_result, p_id);
		// Keep going.
		QueryStorage<I + 1, Cs...>::fetch(p_id, p_mode, r_result);
	}

	static void get_components(SystemExeInfo &r_info, const bool p_force_immutable = false) {
		QueryStorage<I + 1, Cs...>::get_components(r_info);
	}
};

// ---------------------------------------------------------------------- Create

/// `QueryStorage` `Create` filter specialization.
template <std::size_t I, class C, class... Cs>
struct QueryStorage<I, Create<C>, Cs...> : public QueryStorage<I + 1, Cs...> {
	Storage<C> *storage = nullptr;

	QueryStorage(World *p_world) :
			QueryStorage<I + 1, Cs...>(p_world) {
		// Make sure this storage exist.
		p_world->create_storage<C>();
	}

	void initiate_process(World *p_world) {
		QueryStorage<I + 1, Cs...>::initiate_process(p_world);
		storage = p_world->get_storage<C>();
	}

	void conclude_process(World *p_world) {
		QueryStorage<I + 1, Cs...>::conclude_process(p_world);
		storage = nullptr;
	}

	void set_world_notification_active(bool p_active) {
		QueryStorage<I + 1, Cs...>::set_world_notification_active(p_active);
	}

	EntitiesBuffer get_entities() const {
		// This is a NON determinant filter, so just return the other filter.
		return QueryStorage<I + 1, Cs...>::get_entities();
	}

	bool filter_satisfied(EntityID p_entity) const {
		// The `Create` filter never stops the execution.
		return QueryStorage<I + 1, Cs...>::filter_satisfied(p_entity);
	}

	bool can_fetch(EntityID p_entity) const {
		// The `Create` filter allows always to fetch.
		return true;
	}

	template <class... Qs>
	void fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
		if (!storage->has(p_id)) {
			storage->insert(p_id, C());
		}
		set<I>(r_result, storage->get(p_id, p_mode));

		// Keep going.
		QueryStorage<I + 1, Cs...>::fetch(p_id, p_mode, r_result);
	}

	auto get_inner_storage() const {
		return storage;
	}

	static void get_components(SystemExeInfo &r_info, const bool p_force_immutable = false) {
		// Always add as mutable, since the `Crate` filter always require mutability.
		r_info.mutable_components.insert(C::get_component_id());
		// This filter, also use the storage, signal it too.
		r_info.mutable_components_storage.insert(C::get_component_id());
		QueryStorage<I + 1, Cs...>::get_components(r_info);
	}
};

// ----------------------------------------------------------------------- Maybe

/// `QueryStorage` `Maybe` filter specialization.
template <std::size_t I, class C, class... Cs>
struct QueryStorage<I, Maybe<C>, Cs...> : public QueryStorage<I + 1, Cs...> {
	QueryStorage<I, C> query_storage;

	QueryStorage(World *p_world) :
			QueryStorage<I + 1, Cs...>(p_world),
			query_storage(p_world) {}

	void initiate_process(World *p_world) {
		QueryStorage<I + 1, Cs...>::initiate_process(p_world);
		query_storage.initiate_process(p_world);
	}

	void conclude_process(World *p_world) {
		QueryStorage<I + 1, Cs...>::conclude_process(p_world);
		query_storage.conclude_process(p_world);
	}

	void set_world_notification_active(bool p_active) {
		QueryStorage<I + 1, Cs...>::set_world_notification_active(p_active);
		query_storage.set_world_notification_active(p_active);
	}

	EntitiesBuffer get_entities() const {
		// This is a NON determinant filter, so just return the other filter.
		return QueryStorage<I + 1, Cs...>::get_entities();
	}

	bool filter_satisfied(EntityID p_entity) const {
		// The `Maybe` filter never stops the execution.
		return QueryStorage<I + 1, Cs...>::filter_satisfied(p_entity);
	}

	bool can_fetch(EntityID p_entity) const {
		return query_storage.can_fetch(p_entity);
	}

	template <class... Qs>
	void fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
		if (query_storage.filter_satisfied(p_id)) {
			// Ask the sub storage to fetch the data.
			query_storage.fetch(p_id, p_mode, r_result);
		}

		// Keep going.
		QueryStorage<I + 1, Cs...>::fetch(p_id, p_mode, r_result);
	}

	auto get_inner_storage() const {
		return query_storage.get_inner_storage();
	}

	static void get_components(SystemExeInfo &r_info, const bool p_force_immutable = false) {
		QueryStorage<0, C>::get_components(r_info, p_force_immutable);
		QueryStorage<I + 1, Cs...>::get_components(r_info);
	}
};

// -------------------------------------------------------------------------- Not

/// `QueryStorage` `Not` filter specialization.
template <std::size_t I, class C, class... Cs>
struct QueryStorage<I, Not<C>, Cs...> : public QueryStorage<I + 1, Cs...> {
	QueryStorage<I, C> query_storage;

	QueryStorage(World *p_world) :
			QueryStorage<I + 1, Cs...>(p_world),
			query_storage(p_world) {}

	void initiate_process(World *p_world) {
		QueryStorage<I + 1, Cs...>::initiate_process(p_world);
		query_storage.initiate_process(p_world);
	}

	void conclude_process(World *p_world) {
		QueryStorage<I + 1, Cs...>::conclude_process(p_world);
		query_storage.conclude_process(p_world);
	}

	void set_world_notification_active(bool p_active) {
		QueryStorage<I + 1, Cs...>::set_world_notification_active(p_active);
		query_storage.set_world_notification_active(p_active);
	}

	EntitiesBuffer get_entities() const {
		// This is a NON determinant filter, so just return the other filter.
		return QueryStorage<I + 1, Cs...>::get_entities();
	}

	bool filter_satisfied(EntityID p_entity) const {
		// The `Not` filter is satisfied if the sub filter is not satisfied:
		// it's a lot similar to an `!true == false`.
		return query_storage.filter_satisfied(p_entity) == false &&
			   QueryStorage<I + 1, Cs...>::filter_satisfied(p_entity);
	}

	bool can_fetch(EntityID p_entity) const {
		return query_storage.can_fetch(p_entity);
	}

	template <class... Qs>
	void fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
		// If there is something to fetch, fetch it.
		if (query_storage.can_fetch(p_id)) {
			query_storage.fetch(p_id, p_mode, r_result);
		}

		// Keep going.
		QueryStorage<I + 1, Cs...>::fetch(p_id, p_mode, r_result);
	}

	auto get_inner_storage() const {
		return query_storage.get_inner_storage();
	}

	static void get_components(SystemExeInfo &r_info, const bool p_force_immutable = false) {
		// The `Not` collects the data always immutable, so force take it
		// immutably.
		QueryStorage<I, C>::get_components(r_info, true);
		QueryStorage<I + 1, Cs...>::get_components(r_info);
	}
};

// --------------------------------------------------------------------- Changed

/// `QueryStorage` `Changed` filter specialization.
template <std::size_t I, class C, class... Cs>
struct QueryStorage<I, Changed<C>, Cs...> : public QueryStorage<I + 1, Cs...> {
	World *world = nullptr;
	EntityList changed;
	Storage<C> *storage = nullptr;

	QueryStorage(World *p_world) :
			QueryStorage<I + 1, Cs...>(p_world),
			world(p_world) {
		Storage<C> *s = p_world->get_storage<C>();
		if (s) {
			s->add_change_listener(&changed);
		}
	}
	~QueryStorage() {
		Storage<C> *s = world->get_storage<C>();
		if (s) {
			s->remove_change_listener(&changed);
		}
	}

	void initiate_process(World *p_world) {
		QueryStorage<I + 1, Cs...>::initiate_process(p_world);
		storage = p_world->get_storage<C>();
		// Make sure this doesn't change at this point.
		changed.freeze();
	}

	void conclude_process(World *p_world) {
		QueryStorage<I + 1, Cs...>::conclude_process(p_world);
		storage = nullptr;
		// Unfreeze the list and clear it to listen on the new events.
		changed.unfreeze();
		changed.clear();
	}

	void set_world_notification_active(bool p_active) {
		QueryStorage<I + 1, Cs...>::set_world_notification_active(p_active);
		Storage<C> *s = world->get_storage<C>();
		if (s) {
			if (p_active) {
				s->add_change_listener(&changed);
			} else {
				s->remove_change_listener(&changed);
			}
		}
	}

	constexpr static bool is_filter_derminant() {
		// The `Changed` is a determinant filter.
		return true;
	}

	EntitiesBuffer get_entities() const {
		// This is a determinant filter, that iterates over the changed
		// components of this storage.
		const EntitiesBuffer o_entities = QueryStorage<I + 1, Cs...>::get_entities();
		if (unlikely(storage == nullptr)) {
			return o_entities;
		}
		const EntitiesBuffer tmp_entities(changed.size(), changed.get_entities_ptr());
		return tmp_entities.count < o_entities.count ? tmp_entities : o_entities;
	}

	bool filter_satisfied(EntityID p_entity) const {
		if (unlikely(storage == nullptr)) {
			// This is a required field, since there is no storage this can end
			// immediately.
			return false;
		}
		return changed.has(p_entity) && QueryStorage<I + 1, Cs...>::filter_satisfied(p_entity);
	}

	bool can_fetch(EntityID p_entity) const {
		return storage && storage->has(p_entity);
	}

	template <class... Qs>
	void fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
#ifdef DEBUG_ENABLED
		// This can't happen because `is_done` returns true.
		CRASH_COND_MSG(storage == nullptr, "The storage" + String(typeid(Storage<C>).name()) + " is null.");
#endif

		if constexpr (std::is_const<C>::value) {
			set<I>(r_result, const_cast<const Storage<C> *>(storage)->get(p_id, p_mode));
		} else {
			set<I>(r_result, storage->get(p_id, p_mode));
		}

		// Keep going
		QueryStorage<I + 1, Cs...>::fetch(p_id, p_mode, r_result);
	}

	auto get_inner_storage() const {
		return storage;
	}

	static void get_components(SystemExeInfo &r_info, const bool p_force_immutable = false) {
		if (std::is_const<C>::value || p_force_immutable) {
			r_info.immutable_components.insert(C::get_component_id());
		} else {
			r_info.mutable_components.insert(C::get_component_id());
		}
		QueryStorage<I + 1, Cs...>::get_components(r_info);
	}
};

// ----------------------------------------------------------------------- Batch

/// `QueryStorage` `Batch` filter specialization.
template <std::size_t I, class C, class... Cs>
struct QueryStorage<I, Batch<C>, Cs...> : public QueryStorage<I + 1, Cs...> {
	// Here I'm using `0` (instead of `I`) because the data is fetched locally
	// and then parsed by this storage.
	QueryStorage<0, C> query_storage;

	QueryStorage(World *p_world) :
			QueryStorage<I + 1, Cs...>(p_world),
			query_storage(p_world) {}

	void initiate_process(World *p_world) {
		QueryStorage<I + 1, Cs...>::initiate_process(p_world);
		query_storage.initiate_process(p_world);
	}

	void conclude_process(World *p_world) {
		QueryStorage<I + 1, Cs...>::conclude_process(p_world);
		query_storage.conclude_process(p_world);
	}

	void set_world_notification_active(bool p_active) {
		QueryStorage<I + 1, Cs...>::set_world_notification_active(p_active);
		query_storage.set_world_notification_active(p_active);
	}

	EntitiesBuffer get_entities() const {
		return query_storage.get_entities();
	}

	bool filter_satisfied(EntityID p_entity) const {
		return query_storage.filter_satisfied(p_entity);
	}

	bool can_fetch(EntityID p_entity) const {
		return query_storage.can_fetch(p_entity);
	}

	template <class... Qs>
	void fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
		// Fetch the batched data.
		QueryResultTuple<C> tuple;
		query_storage.fetch(p_id, p_mode, tuple);
		fetch_element_type<0, 0, C> data = get<0>(tuple);

		if (query_storage.get_inner_storage() != nullptr && data != nullptr) {
			set<I>(r_result, Batch<fetch_element_type<0, 0, C>>(
									 data,
									 query_storage.get_inner_storage()->get_batch_size(p_id)));
		}

		// Keep going
		QueryStorage<I + 1, Cs...>::fetch(p_id, p_mode, r_result);
	}

	auto get_inner_storage() const {
		return query_storage.get_inner_storage();
	}

	static void get_components(SystemExeInfo &r_info, const bool p_force_immutable = false) {
		QueryStorage<I + 1, C>::get_components(r_info, p_force_immutable);
		QueryStorage<I + 1, Cs...>::get_components(r_info);
	}
};

// ----------------------------------------------------- Any-Join Storage Utility

/// *** Any-Join Storage Utility ***
///
/// This class is used by the filters `Any` and `Join`.
/// We need this because those two filters allow to have multiple components:
/// `Query<Any<TagA, TagB>>` and `Query<Join<TagA, TagB>>`
/// this class deal with the sub components.
///
/// @tparam `I`: 			Stands for `INDEX`, and it's always set to the
/// 						given filter `I` by the `Any` filter or 0 by `Join`.
///
/// @tparam `INCREMENT`: 	This paramenter is used to control if we want to
/// 						index (or not) the sub storages.
/// 						- The filter `Any` set it to 1, so the sub storages
/// 						  can directly set the data inside the tuple.
/// 						- The filter `Join` instead, before to insert the
/// 						  data inside the tuple, has to manipulate it,
/// 						  so it's more convienient keep the storage index
/// 						  to 0.
///
template <std::size_t I, std::size_t INCREMENT, class... Cs>
struct AJUtility {
	/// Used by the `Any` filter, to know how many components there are.
	static constexpr std::size_t LAST_INDEX = I;

	AJUtility(World *p_world) {}

	void initiate_process(World *p_world) {}
	void conclude_process(World *p_world) {}

	void set_world_notification_active(bool p_active) {}

	/// At this point, it's sure all filters are determinant.
	constexpr static bool all_determinant() {
		return true;
	}

	/// At this point, it's sure no determinant filters.
	constexpr static bool any_determinant() {
		return false;
	}

	void get_entities(EntityList &r_entities) const {}

	bool filter_satisfied(EntityID p_entity) const {
		// Not satisfied.
		return false;
	}

	bool can_fetch(EntityID p_entity) const {
		// Not satisfied.
		return false;
	}

	template <class... Qs>
	void any_fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {}
	JoinData join_fetch(EntityID p_id, Space p_mode) const {
		return JoinData(nullptr, godex::COMPONENT_NONE, true);
	}
};

template <std::size_t I, std::size_t INCREMENT, class C, class... Cs>
struct AJUtility<I, INCREMENT, C, Cs...> : AJUtility<I + INCREMENT, INCREMENT, Cs...> {
	QueryStorage<I, C> storage;

	AJUtility(World *p_world) :
			AJUtility<I + INCREMENT, INCREMENT, Cs...>(p_world),
			storage(p_world) {}

	void initiate_process(World *p_world) {
		AJUtility<I + INCREMENT, INCREMENT, Cs...>::initiate_process(p_world);
		storage.initiate_process(p_world);
	}

	void conclude_process(World *p_world) {
		AJUtility<I + INCREMENT, INCREMENT, Cs...>::conclude_process(p_world);
		storage.conclude_process(p_world);
	}

	void set_world_notification_active(bool p_active) {
		AJUtility<I + INCREMENT, INCREMENT, Cs...>::set_world_notification_active(p_active);
		storage.set_world_notification_active(p_active);
	}

	/// Return `true` when all subfilters are terminant.
	constexpr static bool all_determinant() {
		if constexpr (QueryStorage<I, C>::is_filter_derminant()) {
			return AJUtility<I + INCREMENT, INCREMENT, Cs...>::all_determinant();
		} else {
			return false;
		}
	}

	/// Return `true` if at least one is determinant.
	constexpr static bool any_determinant() {
		if constexpr (QueryStorage<I, C>::is_filter_derminant()) {
			return true;
		} else {
			// Keep search
			return AJUtility<I + INCREMENT, INCREMENT, Cs...>::any_determinant();
		}
	}

	void get_entities(EntityList &r_entities) const {
		if (storage.get_entities().count != UINT32_MAX) {
			// This is a determinant fitler, take the `Entities`.
			for (uint32_t i = 0; i < storage.get_entities().count; i += 1) {
				r_entities.insert(storage.get_entities().entities[i]);
			}
		}
		AJUtility<I + INCREMENT, INCREMENT, Cs...>::get_entities(r_entities);
	}

	bool filter_satisfied(EntityID p_entity) const {
		// Is someone able to satisfy the filter?
		if (storage.filter_satisfied(p_entity)) {
			// Satisfied.
			return true;
		} else {
			// Not in storage, try the next one.
			return AJUtility<I + INCREMENT, INCREMENT, Cs...>::filter_satisfied(p_entity);
		}
	}

	bool can_fetch(EntityID p_entity) const {
		// Is someone able to fetch?
		if (storage.can_fetch(p_entity)) {
			// Satisfied.
			return true;
		} else {
			// Not in storage, try the next one.
			return AJUtility<I + INCREMENT, INCREMENT, Cs...>::can_fetch(p_entity);
		}
	}

	/// Used by `Any` to fetch the data.
	/// Fetches all the components if there is something to fetch.
	template <class... Qs>
	void any_fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
		if (storage.can_fetch(p_id)) {
			// There is something to fetch, fetch it.
			storage.fetch(p_id, p_mode, r_result);
		}
		AJUtility<I + INCREMENT, INCREMENT, Cs...>::any_fetch(p_id, p_mode, r_result);
	}

	/// Used by `Join` to fetch the data.
	/// Fetches just the first one that exist.
	JoinData join_fetch(EntityID p_id, Space p_mode) const {
		if (storage.can_fetch(p_id)) {
			// There is something to fetch, fetch it.
			QueryResultTuple<C> tuple;
			storage.fetch(p_id, p_mode, tuple);
			return JoinData(
					(void *)get<0>(tuple),
					std::remove_pointer_t<fetch_element_type<0, 0, C>>::get_component_id(),
					std::is_const_v<std::remove_pointer_t<fetch_element_type<0, 0, C>>>);
		} else {
			return AJUtility<0, 0, Cs...>::join_fetch(p_id, p_mode);
		}
	}
};

// -------------------------------------------------------------------------- Any

/// `QueryStorage` `Any` filter specialization.
///
/// Returns all the components if at least one satisfy the sub filter.
///
/// Notice: You can't specify `Any` as sub component: `Query<Any<Any<Transform>>>`
/// though, you can specify `Join`: `Query<Any<Join<TagA, TagB>>>`
template <std::size_t I, class... C, class... Cs>
struct QueryStorage<I, Any<C...>, Cs...> : QueryStorage<AJUtility<I, 1, C...>::LAST_INDEX, Cs...> {
	AJUtility<I, 1, C...> sub_storages;

	EntityList entities;

	QueryStorage(World *p_world) :
			QueryStorage<AJUtility<I, 1, C...>::LAST_INDEX, Cs...>(p_world),
			sub_storages(p_world) {}

	void initiate_process(World *p_world) {
		QueryStorage<AJUtility<I, 1, C...>::LAST_INDEX, Cs...>::initiate_process(p_world);
		sub_storages.initiate_process(p_world);

		entities.clear();
		// TODO Can we cache this somehow?
		// 	track of the biggest `EntityID` it has.
		// 	So the `entities` list we have here could be prepared, and a lot of
		// 	reallocation can be avoided.
		sub_storages.get_entities(entities);
	}

	void conclude_process(World *p_world) {
		QueryStorage<AJUtility<I, 1, C...>::LAST_INDEX, Cs...>::conclude_process(p_world);
		sub_storages.conclude_process(p_world);
	}

	void set_world_notification_active(bool p_active) {
		QueryStorage<AJUtility<I, 1, C...>::LAST_INDEX, Cs...>::set_world_notification_active(p_active);
		sub_storages.set_world_notification_active(p_active);
	}

	constexpr static bool is_filter_derminant() {
		// The `Any` storage is a determinant filter if at least one subfilter is.
		return AJUtility<I, 1, C...>::any_determinant();
	}

	EntitiesBuffer get_entities() const {
		if constexpr (is_filter_derminant()) {
			return EntitiesBuffer(entities.size(), entities.get_entities_ptr());
		} else {
			return EntitiesBuffer(UINT32_MAX, nullptr);
		}
	}

	bool filter_satisfied(EntityID p_entity) const {
		// Just check if we have this entity.
		if constexpr (AJUtility<I, 1, C...>::all_determinant()) {
			// Since all the filters are determinant, we can just check this:
			return entities.has(p_entity);
		} else {
			// Not all filters are determinant, so we need to check one by one,
			// because even a not determinant filter may satisfy the process.
			return sub_storages.filter_satisfied(p_entity);
		}
	}

	bool can_fetch(EntityID p_entity) const {
		return sub_storages.can_fetch(p_entity);
	}

	template <class... Qs>
	void fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
		// Fetch the data from the sub storages.
		sub_storages.any_fetch(p_id, p_mode, r_result);

		// Keep going.
		QueryStorage<AJUtility<I, 1, C...>::LAST_INDEX, Cs...>::fetch(p_id, p_mode, r_result);
	}

	// TODO how? here we have multiple storages.
	// auto get_inner_storage() const {
	//	return query_storage.get_inner_storage();
	//}

	static void get_components(SystemExeInfo &r_info, const bool p_force_immutable = false) {
		// Take the components storages used, taking advantage of the
		// get_components function.
		QueryStorage<0, C...>::get_components(r_info, p_force_immutable);
		QueryStorage<0, Cs...>::get_components(r_info);
	}
};

// ------------------------------------------------------------------------- Join

/// `QueryStorage` `Join`.
///
/// Returns the first component that satisfy the sub filters.
///
/// Notice: You can't specify `Any` as sub filter: `Query<Join<Any<Transform>>>`
template <std::size_t I, class... C, class... Cs>
struct QueryStorage<I, Join<C...>, Cs...> : QueryStorage<I + 1, Cs...> {
	AJUtility<0, 0, C...> sub_storages;

	EntityList entities;

	QueryStorage(World *p_world) :
			QueryStorage<I + 1, Cs...>(p_world),
			sub_storages(p_world) {}

	void initiate_process(World *p_world) {
		QueryStorage<I + 1, Cs...>::initiate_process(p_world);
		sub_storages.initiate_process(p_world);

		entities.clear();
		// TODO Exist a way to cache this?
		// 	track of the biggest `EntityID` it has.
		// 	So the `entities` list we have here could be prepared, and a lot of
		// 	reallocation can be avoided.
		sub_storages.get_entities(entities);
	}

	void conclude_process(World *p_world) {
		QueryStorage<I + 1, Cs...>::conclude_process(p_world);
		sub_storages.conclude_process(p_world);
	}

	void set_world_notification_active(bool p_active) {
		QueryStorage<I + 1, Cs...>::set_world_notification_active(p_active);
		sub_storages.set_world_notification_active(p_active);
	}

	constexpr static bool is_filter_derminant() {
		// The `Join` storage is a determinant filter if at least one subfilter is.
		return AJUtility<0, 0, C...>::any_determinant();
	}

	EntitiesBuffer get_entities() const {
		if constexpr (is_filter_derminant()) {
			return EntitiesBuffer(entities.size(), entities.get_entities_ptr());
		} else {
			return EntitiesBuffer(UINT32_MAX, nullptr);
		}
	}

	bool filter_satisfied(EntityID p_entity) const {
		// Just check if we have this entity.
		if constexpr (AJUtility<0, 0, C...>::all_determinant()) {
			// Since all the filters are determinant, we can just check this:
			return entities.has(p_entity);
		} else {
			// Not all filters are determinant, so we need to check one by one,
			// because even a not determinant filter may satisfy the process.
			return sub_storages.filter_satisfied(p_entity);
		}
	}

	bool can_fetch(EntityID p_entity) const {
		return sub_storages.can_fetch(p_entity);
	}

	template <class... Qs>
	void fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
		// Fetch the data from the sub storages.
		set<I>(r_result, sub_storages.join_fetch(p_id, p_mode));

		// Keep going.
		QueryStorage<I + 1, Cs...>::fetch(p_id, p_mode, r_result);
	}

	// TODO how? here we have multiple storages.
	// auto get_inner_storage() const {
	//	return query_storage.get_inner_storage();
	//}

	static void get_components(SystemExeInfo &r_info, const bool p_force_immutable = false) {
		// Take the components storages used, taking advantage of the
		// get_components function.
		QueryStorage<0, C...>::get_components(r_info, p_force_immutable);
		QueryStorage<0, Cs...>::get_components(r_info);
	}
};

// -------------------------------------------------------------------- No filter

/// `QueryStorage` no filter specialization.
template <std::size_t I, class C, class... Cs>
struct QueryStorage<I, C, Cs...> : QueryStorage<I + 1, Cs...> {
	Storage<C> *storage = nullptr;

	QueryStorage(World *p_world) :
			QueryStorage<I + 1, Cs...>(p_world) {}

	void initiate_process(World *p_world) {
		QueryStorage<I + 1, Cs...>::initiate_process(p_world);
		storage = p_world->get_storage<C>();
	}

	void conclude_process(World *p_world) {
		QueryStorage<I + 1, Cs...>::conclude_process(p_world);
		storage = nullptr;
	}

	void set_world_notification_active(bool p_active) {
		QueryStorage<I + 1, Cs...>::set_world_notification_active(p_active);
	}

	constexpr static bool is_filter_derminant() {
		// The `With` is a determinant filter.
		return true;
	}

	EntitiesBuffer get_entities() const {
		// This is a determinant filter, that iterates over the existing
		// components of this storage.
		const EntitiesBuffer o_entities = QueryStorage<I + 1, Cs...>::get_entities();
		if (unlikely(storage == nullptr)) {
			return o_entities;
		}
		const EntitiesBuffer tmp_entities = storage->get_stored_entities();
		return tmp_entities.count < o_entities.count ? tmp_entities : o_entities;
	}

	bool filter_satisfied(EntityID p_entity) const {
		if (unlikely(storage == nullptr)) {
			// This is a required field, since there is no storage this can end
			// immediately.
			return false;
		}
		return storage->has(p_entity) && QueryStorage<I + 1, Cs...>::filter_satisfied(p_entity);
	}

	bool can_fetch(EntityID p_entity) const {
		return storage && storage->has(p_entity);
	}

	template <class... Qs>
	void fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
#ifdef DEBUG_ENABLED
		// This can't happen because `is_done` returns true.
		CRASH_COND_MSG(storage == nullptr, "The storage" + String(typeid(Storage<C>).name()) + " is null.");
#endif

		// Set the `Component` inside th tuple.
		if constexpr (std::is_const<C>::value) {
			set<I>(r_result, const_cast<const Storage<C> *>(storage)->get(p_id, p_mode));
		} else {
			set<I>(r_result, storage->get(p_id, p_mode));
		}

		// Keep going.
		QueryStorage<I + 1, Cs...>::fetch(p_id, p_mode, r_result);
	}

	auto get_inner_storage() const {
		return storage;
	}

	static void get_components(SystemExeInfo &r_info, const bool p_force_immutable = false) {
		if (std::is_const<C>::value || p_force_immutable) {
			r_info.immutable_components.insert(C::get_component_id());
		} else {
			r_info.mutable_components.insert(C::get_component_id());
		}
		QueryStorage<I + 1, Cs...>::get_components(r_info);
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
	QueryStorage<0, Cs...> q;

public:
	Query(World *p_world) :
			q(p_world) {
	}

	void initiate_process(World *p_world) {
		m_space = LOCAL;
		q.initiate_process(p_world);

		// Prepare the query:
		// Ask all the pointed storage to return a list of entities to iterate;
		// the query, takes the smallest one, and iterates over it.
		entities = q.get_entities();
		if (unlikely(entities.count == UINT32_MAX)) {
			entities.count = 0;
			ERR_PRINT("This query is not valid, you are using only non determinant fileters (like `Not` and `Maybe`).");
		}
	}

	void conclude_process(World *p_world) {
		q.conclude_process(p_world);
	}

	void set_world_notification_active(bool p_active) {
		q.set_world_notification_active(p_active);
	}

	struct Iterator {
		using iterator_category = std::forward_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = QueryResultTuple<Cs...>;

		Iterator(Query<Cs...> *p_query, const EntityID *p_entity) :
				query(p_query), entity(p_entity) {}

		bool is_valid() const {
			return *this != query->end();
		}

		value_type fetch() const {
			return operator*();
		}

		value_type operator*() const {
			QueryResultTuple<Cs...> result;
			query->q.fetch(*entity, query->m_space, result);
			return result;
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
	QueryResultTuple<Cs...> operator[](EntityID p_entity) {
		QueryResultTuple<Cs...> result;
		q.fetch(p_entity, m_space, result);
		return result;
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
		QueryStorage<0, Cs...>::get_components(r_info);
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
