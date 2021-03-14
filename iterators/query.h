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

// TODO remove this.
template <class... Cs>
struct Group {};

// TODO remove all these
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

template <typename T>
using to_query_return_type_t = typename to_query_return_type<T>::type;

template <typename... Ts>
struct to_query_return_type<Group<Ts...>> {
	typedef std::tuple<to_query_return_type_t<Ts>...> type;
};

template <typename T>
struct to_query_return_type<Batch<T>> {
	typedef Batch<to_query_return_type_t<T>> type;
};

// ---------------------------------------------------------- Fetch Elements Type

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
/// fetch_element_type<0, 0, Without<int>, Any<int, Batch<Changed<float>>>, Maybe<bool>, EntityID> // This is int*
/// fetch_element_type<1, 0, Without<int>, Any<int, Batch<Changed<float>>>, Maybe<bool>, EntityID> // This is int*
/// fetch_element_type<2, 0, Without<int>, Any<int, Batch<Changed<float>>>, Maybe<bool>, EntityID> // This is Batch<float*>
/// fetch_element_type<3, 0, Without<int>, Any<int, Batch<Changed<float>>>, Maybe<bool>, EntityID> // This is float*
/// fetch_element_type<4, 0, Without<int>, Any<int, Batch<Changed<float>>>, Maybe<bool>, EntityID> // This is bool*
/// ```
///
/// # How it works
/// When you call `fetch_element_type`, as shown, the compiler fetches the `type`
/// of the element where `S` and `I` are the same.
///
/// @param S stands for `Search`
/// @param I stands for `Index`, this must always be 0 when you define it.
/// @param ...Cs The Query type.
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

/// We found the `Without` filter, fetch the type now.
template <std::size_t S, class C, class... Cs>
struct fetch_element<S, S, Without<C>, Cs...> : fetch_element<S, S + 1, Cs...> {
	// Keep search the sub type: so we can nest with other filters.
	using type = fetch_element_type<0, 0, C>;
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

// ----------------------------------------------------------- Query Result Tuple

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
/// This template is able to flatten the filters: `Changed`, `Without`, `Maybe`, `Any`, `Join`
///
/// Notice: This is just forwarding the declaration, indeed this has the same
/// index the flattened data has.
///
/// ```
/// This:
/// QeryResultTuple_Impl<0, int, Without<float>>
///
/// Is compiled as:
/// `ResultTuple<0, int> :
/// 		 ResultTuple<1, Without<>> :
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

/// Skip the filter `Changed`.
template <std::size_t S, class T, class C, class... Cs>
constexpr void set_impl(T p_val, QueryResultTuple_Impl<S, Changed<C>, Cs...> &tuple) noexcept {
	// Forward to subfilters.
	set_impl<S>(p_val, static_cast<QueryResultTuple_Impl<S, C, Cs...> &>(tuple));
}

/// Skip the filter `Without`.
template <std::size_t S, class T, class C, class... Cs>
constexpr void set_impl(T p_val, QueryResultTuple_Impl<S, Without<C>, Cs...> &tuple) noexcept {
	// Forward to subfilters.
	set_impl<S>(p_val, static_cast<QueryResultTuple_Impl<S, C, Cs...> &>(tuple));
}

/// Skip the filter `Maybe`.
template <std::size_t S, class T, class C, class... Cs>
constexpr void set_impl(T p_val, QueryResultTuple_Impl<S, Maybe<C>, Cs...> &tuple) noexcept {
	// Forward to subfilters.
	set_impl<S>(p_val, static_cast<QueryResultTuple_Impl<S, C, Cs...> &>(tuple));
}

/// Skip the filter `Any`.
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

/// Skip the filter `Without`.
template <std::size_t S, class C, class... Cs>
constexpr auto get_impl(const QueryResultTuple_Impl<S, Without<C>, Cs...> &tuple) noexcept {
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

// TODO remove this once the Query is refactored
namespace REMOVE_THIS_NS {
/// Can be used to fetch the data from a tuple.
template <std::size_t S, class... Cs>
constexpr fetch_element_type<S, 0, Cs...> get(const QueryResultTuple<Cs...> &tuple) noexcept {
	return get_impl<S>(tuple);
}
} // namespace REMOVE_THIS_NS

// --------------------------------------------------------------- Query Storages

/// `QueryStorage` specialization with 0 template arguments.
template <std::size_t I, class... Cs>
struct QueryStorage {
	QueryStorage(World *p_world) {}

	EntitiesBuffer get_entities() const {
		// This is a NON determinant filter, so just return UINT32_MAX.
		return { UINT32_MAX, nullptr };
	}

	bool filter_satisfied(EntityID p_entity) const { return true; }
	std::tuple<to_query_return_type_t<Cs>...> get(EntityID p_id, Space p_mode) const { return std::tuple(); }

	template <class... Qs>
	void new_fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
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

	EntitiesBuffer get_entities() const {
		return QueryStorage<I + 1, Cs...>::get_entities();
	}

	bool filter_satisfied(EntityID p_entity) const {
		return QueryStorage<I + 1, Cs...>::filter_satisfied(p_entity);
	}

	std::tuple<EntityID, to_query_return_type_t<Cs>...> get(EntityID p_id, Space p_mode) {
		return std::tuple_cat(
				std::tuple<EntityID>(p_id),
				QueryStorage<I + 1, Cs...>::get(p_id, p_mode));
	}

	template <class... Qs>
	void new_fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
		// Set the `EntityID` at position `I`.
		set<I>(r_result, p_id);
		// Keep going.
		QueryStorage<I + 1, Cs...>::new_fetch(p_id, p_mode, r_result);
	}

	static void get_components(SystemExeInfo &r_info, const bool p_force_immutable = false) {
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
			query_storage(p_world) {
	}

	EntitiesBuffer get_entities() const {
		// This is a NON determinant filter, so just return the other filter.
		return QueryStorage<I + 1, Cs...>::get_entities();
	}

	bool filter_satisfied(EntityID p_entity) const {
		// The `Maybe` filter never stops the execution.
		return QueryStorage<I + 1, Cs...>::filter_satisfied(p_entity);
	}

	std::tuple<C *, to_query_return_type_t<Cs>...> get(EntityID p_id, Space p_mode) const {
		if (query_storage.filter_satisfied(p_id)) {
			// TODO assume it's correct, but this is not nesting.
			return std::tuple_cat(
					std::tuple<C *>(query_storage.get(p_id, p_mode)),
					QueryStorage<I + 1, Cs...>::get(p_id, p_mode));
		} else {
			// Nothing to fetch, just set nullptr.
			return std::tuple_cat(
					std::tuple<C *>(nullptr),
					QueryStorage<I + 1, Cs...>::get(p_id, p_mode));
		}
	}

	template <class... Qs>
	void new_fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
		if (query_storage.filter_satisfied(p_id)) {
			// Ask the sub storage to fetch the data.
			query_storage.new_fetch(p_id, p_mode, r_result);
		}

		// Keep going.
		QueryStorage<I + 1, Cs...>::new_fetch(p_id, p_mode, r_result);
	}

	auto get_inner_storage() const {
		return query_storage.get_inner_storage();
	}

	static void get_components(SystemExeInfo &r_info, const bool p_force_immutable = false) {
		QueryStorage<0, C>::get_components(r_info, p_force_immutable);
		QueryStorage<I + 1, Cs...>::get_components(r_info);
	}
};

// --------------------------------------------------------------------- Without

/// `QueryStorage` `Without` filter specialization.
template <std::size_t I, class C, class... Cs>
struct QueryStorage<I, Without<C>, Cs...> : public QueryStorage<I + 1, Cs...> {
	QueryStorage<I, C> query_storage;

	QueryStorage(World *p_world) :
			QueryStorage<I + 1, Cs...>(p_world),
			query_storage(p_world) {
	}

	EntitiesBuffer get_entities() const {
		// This is a NON determinant filter, so just return the other filter.
		return QueryStorage<I + 1, Cs...>::get_entities();
	}

	bool filter_satisfied(EntityID p_entity) const {
		// The `Without` filter is satisfied if the sub filter is not satisfied:
		// it's a lot similar to a `Not` or `!`.
		return query_storage.filter_satisfied(p_entity) == false &&
			   QueryStorage<I + 1, Cs...>::filter_satisfied(p_entity);
	}

	std::tuple<C *, to_query_return_type_t<Cs>...> get(EntityID p_id, Space p_mode) const {
		// Just keep going, the `Without` filter doesn't collect data.
		return std::tuple_cat(std::tuple<C *>(nullptr), QueryStorage<I + 1, Cs...>::get(p_id, p_mode));
	}

	template <class... Qs>
	void new_fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
		// Nothing to do, the `Without` filter doesn't fetches.
		// Just keep going.
		QueryStorage<I + 1, Cs...>::new_fetch(p_id, p_mode, r_result);
	}

	auto get_inner_storage() const {
		return query_storage.get_inner_storage();
	}

	static void get_components(SystemExeInfo &r_info, const bool p_force_immutable = false) {
		// The `Without` collects the data always immutable, so force take it
		// immutably.
		QueryStorage<I, C>::get_components(r_info, true);
		QueryStorage<I + 1, Cs...>::get_components(r_info);
	}
};

// --------------------------------------------------------------------- Changed

/// `QueryStorage` `Changed` filter specialization.
template <std::size_t I, class C, class... Cs>
struct QueryStorage<I, Changed<C>, Cs...> : public QueryStorage<I + 1, Cs...> {
	Storage<C> *storage = nullptr;

	QueryStorage(World *p_world) :
			QueryStorage<I + 1, Cs...>(p_world),
			storage(p_world->get_storage<C>()) {
	}

	EntitiesBuffer get_entities() const {
		// This is a determinant filter, that iterates over the changed
		// components of this storage.
		const EntitiesBuffer o_entities = QueryStorage<I + 1, Cs...>::get_entities();
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
		return storage->is_changed(p_entity) && QueryStorage<I + 1, Cs...>::filter_satisfied(p_entity);
	}

	std::tuple<C *, to_query_return_type_t<Cs>...> get(EntityID p_id, Space p_mode) const {
#ifdef DEBUG_ENABLED
		// This can't happen because `is_done` returns true.
		CRASH_COND_MSG(storage == nullptr, "The storage" + String(typeid(Storage<C>).name()) + " is null.");
#endif

		if constexpr (std::is_const<C>::value) {
			return std::tuple_cat(
					std::tuple<C *>(const_cast<const Storage<C> *>(storage)->get(p_id, p_mode)),
					QueryStorage<I + 1, Cs...>::get(p_id, p_mode));
		} else {
			return std::tuple_cat(
					std::tuple<C *>(storage->get(p_id, p_mode)),
					QueryStorage<I + 1, Cs...>::get(p_id, p_mode));
		}
	}

	template <class... Qs>
	void new_fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
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
		QueryStorage<I + 1, Cs...>::new_fetch(p_id, p_mode, r_result);
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
		r_info.need_changed.insert(C::get_component_id());
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
				QueryStorage<I + 1, Cs...>::get(p_id, p_mode));
	}

	template <class... Qs>
	void new_fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
		// Fetch the batched data.
		QueryResultTuple<C> tuple;
		query_storage.new_fetch(p_id, p_mode, tuple);
		fetch_element_type<0, 0, C> data = REMOVE_THIS_NS::get<0>(tuple);

		if (query_storage.get_inner_storage() != nullptr && data != nullptr) {
			set<I>(r_result, Batch<fetch_element_type<0, 0, C>>(
									 data,
									 query_storage.get_inner_storage()->get_batch_size(p_id)));
		}

		// Keep going
		QueryStorage<I + 1, Cs...>::new_fetch(p_id, p_mode, r_result);
	}

	auto get_inner_storage() const {
		return query_storage.get_inner_storage();
	}

	static void get_components(SystemExeInfo &r_info, const bool p_force_immutable = false) {
		QueryStorage<I + 1, C>::get_components(r_info, p_force_immutable);
		QueryStorage<I + 1, Cs...>::get_components(r_info);
	}
};

// ------------------------------------------------------------------------- Any

/*
template <class... Cs>
struct AnyStorage {
	AnyStorage(World *p_world) {}

	void get_entities(EntitiesBuffer r_buffers[], uint32_t p_index = 0) const {}
	bool filter_satisfied(EntityID p_entity) const { return false; }
	AnyData get(EntityID p_id, Space p_mode) const { return AnyData(nullptr, godex::COMPONENT_NONE, true); }
};

template <class C, class... Cs>
struct AnyStorage<C, Cs...> : AnyStorage<Cs...> {
	QueryStorage<0, C> storage;

	AnyStorage(World *p_world) :
			AnyStorage<Cs...>(p_world),
			storage(p_world) {
	}

	void get_entities(EntitiesBuffer r_buffers[], uint32_t p_index = 0) const {
		r_buffers[p_index] = storage.get_entities();
		AnyStorage<Cs...>::get_entities(r_buffers, p_index + 1);
	}

	bool filter_satisfied(EntityID p_entity) const {
		if (storage.filter_satisfied(p_entity)) {
			// Found in storage, stop here.
			return true;
		} else {
			// Not in storage, try the next one.
			return AnyStorage<Cs...>::filter_satisfied(p_entity);
		}
	}

	AnyData get(EntityID p_id, Space p_mode) const {
		if (storage.filter_satisfied(p_id)) {
			auto [d] = storage.get(p_id, p_mode);
			if constexpr (std::is_const<std::remove_pointer_t<to_query_return_type_t<C>>>::value) {
				return AnyData(
						const_cast<void *>(static_cast<const void *>(d)),
						std::remove_pointer_t<to_query_return_type_t<C>>::get_component_id(),
						true);
			} else {
				return AnyData(
						static_cast<void *>(d),
						std::remove_pointer_t<to_query_return_type_t<C>>::get_component_id(),
						false);
			}
		} else {
			return AnyStorage<Cs...>::get(p_id, p_mode);
		}
	}
};

/// `QueryStorage` `Any` immutable filter specialization.
template <std::size_t I, class... C, class... Cs>
struct QueryStorage<I, Any<C...>, Cs...> : QueryStorage<I + 1, Cs...> {
	constexpr static std::size_t storages_count = sizeof...(C);
	AnyStorage<C...> flat_storages;

	EntityID *entities_data = nullptr;
	EntitiesBuffer entities_buffer;

	QueryStorage(World *p_world) :
			QueryStorage<I + 1, Cs...>(p_world),
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

	std::tuple<AnyData, to_query_return_type_t<Cs>...> get(EntityID p_id, Space p_mode) const {
		return std::tuple_cat(
				std::tuple<AnyData>(flat_storages.get(p_id, p_mode)),
				QueryStorage<I + 1, Cs...>::get(p_id, p_mode));
	}

	//template <std::size_t I, class... Qs>
	//void new_fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
	//	// TODO
	//}

	//auto get_inner_storage() const {
	//	return query_storage.get_inner_storage();
	//}

	static void get_components(SystemExeInfo &r_info, const bool p_force_immutable = false) {
		// Take the components storages used, taking advantage of the
		// get_components function.
		QueryStorage<0, C...>::get_components(r_info, p_force_immutable);
		QueryStorage<I + 1, Cs...>::get_components(r_info);
	}
};
*/

// -------------------------------------------------------------------- No filter

/// `QueryStorage` no filter specialization.
template <std::size_t I, class C, class... Cs>
struct QueryStorage<I, C, Cs...> : QueryStorage<I + 1, Cs...> {
	Storage<C> *storage = nullptr;

	QueryStorage(World *p_world) :
			QueryStorage<I + 1, Cs...>(p_world),
			storage(p_world->get_storage<C>()) {
	}

	EntitiesBuffer get_entities() const {
		// This is a determinant filter, that iterates over the existing
		// components of this storage.
		const EntitiesBuffer o_entities = QueryStorage<I + 1, Cs...>::get_entities();
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
		return storage->has(p_entity) && QueryStorage<I + 1, Cs...>::filter_satisfied(p_entity);
	}

	std::tuple<C *, to_query_return_type_t<Cs>...> get(EntityID p_id, Space p_mode) const {
#ifdef DEBUG_ENABLED
		// This can't happen because `is_done` returns true.
		CRASH_COND_MSG(storage == nullptr, "The storage" + String(typeid(Storage<C>).name()) + " is null.");
#endif

		if constexpr (std::is_const<C>::value) {
			return std::tuple_cat(
					std::tuple<C *>(const_cast<const Storage<C> *>(storage)->get(p_id, p_mode)),
					QueryStorage<I + 1, Cs...>::get(p_id, p_mode));
		} else {
			return std::tuple_cat(
					std::tuple<C *>(storage->get(p_id, p_mode)),
					QueryStorage<I + 1, Cs...>::get(p_id, p_mode));
		}
	}

	template <class... Qs>
	void new_fetch(EntityID p_id, Space p_mode, QueryResultTuple<Qs...> &r_result) const {
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
		QueryStorage<I + 1, Cs...>::new_fetch(p_id, p_mode, r_result);
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

	QueryResultTuple<Cs...> new_get(EntityID p_entity) {
		QueryResultTuple<Cs...> result;
		q.new_fetch(p_entity, m_space, result);
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
