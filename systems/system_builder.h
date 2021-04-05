#pragma once

#include "../databags/databag.h"
#include "../iterators/query.h"
#include "../spawners/spawner.h"
#include <type_traits>

// TODO put all this into a CPP or a namespace?

namespace SystemBuilder {

/// Used to fetch the system function arguments:
template <class... Cs>
struct InfoConstructor {
	InfoConstructor(SystemExeInfo &r_info) {}
};

/// Fetches the component used by this `Storage`.
/// The component storage can be taken only as mutable (non const) pointer.
/// ```
/// void test_func(Storage<Component> *p_component_storage){}
/// ```
template <class C, class... Cs>
struct InfoConstructor<Storage<C> *, Cs...> : InfoConstructor<Cs...> {
	InfoConstructor(SystemExeInfo &r_info) :
			InfoConstructor<Cs...>(r_info) {
		r_info.mutable_components_storage.insert(C::get_component_id());
	}
};

/// Fetches the components used by this Spawner.
template <class I, class... Cs>
struct InfoConstructor<Spawner<I> &, Cs...> : InfoConstructor<Cs...> {
	InfoConstructor(SystemExeInfo &r_info) :
			InfoConstructor<Cs...>(r_info) {
		Spawner<I>::get_components(r_info);
	}
};

/// Fetches the argument `Query`.
/// The query is supposed to be a mutable query reference:
/// ```
/// void test_func(Query<const Component> &query){}
/// ```
template <class... Qcs, class... Cs>
struct InfoConstructor<Query<Qcs...> &, Cs...> : InfoConstructor<Cs...> {
	InfoConstructor(SystemExeInfo &r_info) :
			InfoConstructor<Cs...>(r_info) {
		Query<Qcs...>::get_components(r_info);
	}
};

/// Fetches the `Databag`.
/// The `Databag` can be taken as mutable or immutable pointer.
/// ```
/// void test_func(const FrameTimeDatabag *p_frame_time){}
/// ```
template <class D, class... Cs>
struct InfoConstructor<D *, Cs...> : InfoConstructor<Cs...> {
	InfoConstructor(SystemExeInfo &r_info) :
			InfoConstructor<Cs...>(r_info) {
		// Databag
		if (std::is_const<D>()) {
			r_info.immutable_databags.insert(D::get_databag_id());
		} else {
			r_info.mutable_databags.insert(D::get_databag_id());
		}
	}
};

/// Creates a SystemExeInfo, extracting the information from a system function.
template <class... RCs>
void get_system_info_from_function(SystemExeInfo &r_info, void (*system_func)(RCs...)) {
	InfoConstructor<RCs...> a(r_info);
}

/// `DataFetcher` is used to fetch the data from the world and provide it to the
/// `System`.
template <class C>
struct DataFetcher {};

/// Storage
template <class C>
struct DataFetcher<Storage<C> *> {
	Storage<C> *inner;

	DataFetcher(World *p_world) :
			inner(p_world->get_storage<C>()) {}
};

/// Spawner
template <class I>
struct DataFetcher<Spawner<I> &> {
	Spawner<I> inner;

	DataFetcher(World *p_world) :
			inner(p_world) {}
};

/// Query
template <class... Cs>
struct DataFetcher<Query<Cs...> &> {
	Query<Cs...> inner;

	DataFetcher(World *p_world) :
			inner(p_world) {}
};

/// Databag
template <class D>
struct DataFetcher<D *> {
	D *inner;

	DataFetcher(World *p_world) {
		inner = p_world->get_databag<D>();
	}
};

#define OBTAIN(name, T, world) auto name = DataFetcher<T>(world);

// ~~~~ system_exec_func definition ~~~~ //
#include "system_exe_funcs.gen.h"

// ~~~~ temporary_system_exec_func definition ~~~~ //
#include "temporary_system_exe_funcs.gen.h"

#undef OBTAIN
} // namespace SystemBuilder
