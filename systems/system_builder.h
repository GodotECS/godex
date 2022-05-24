#pragma once

#include "../databags/databag.h"
#include "../iterators/events_emitter_receiver.h"
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

/// Fetches the `EventsEmitter`.
/// The `EventsEmitter` is supposed to be takes as mutable reference.
/// ```
/// void test_func(const EventsEmitter<MyEventType> &p_emitter){}
/// ```
template <class E, class... Cs>
struct InfoConstructor<EventsEmitter<E> &, Cs...> : InfoConstructor<Cs...> {
	InfoConstructor(SystemExeInfo &r_info) :
			InfoConstructor<Cs...>(r_info) {
		r_info.events_emitters.insert(E::get_event_id());
	}
};

/// Fetches the `Events`.
/// The `Events` is supposed to be takes as mutable reference.
/// ```
/// void test_func(const EventsReceiver<MyEventType, EMITTER(EmitterName)> &p_events){}
/// ```
template <class E, typename EmitterName, class... Cs>
struct InfoConstructor<EventsReceiver<E, EmitterName> &, Cs...> : InfoConstructor<Cs...> {
	InfoConstructor(SystemExeInfo &r_info) :
			InfoConstructor<Cs...>(r_info) {
		RBSet<String> *emitters = r_info.events_receivers.lookup_ptr(E::get_event_id());
		if (emitters == nullptr) {
			r_info.events_receivers.insert(E::get_event_id(), RBSet<String>());
			emitters = r_info.events_receivers.lookup_ptr(E::get_event_id());
		}
		emitters->insert(EmitterName::data());
	}
};

/// Creates a SystemExeInfo, extracting the information from a system function.
template <class... RCs>
void get_system_info_from_function(SystemExeInfo &r_info, void (*system_func)(RCs...)) {
	InfoConstructor<RCs...> a(r_info);
}

/// Creates a SystemExeInfo, extracting the information from a system dispatcher function.
template <class... RCs>
void get_system_info_from_function(SystemExeInfo &r_info, uint32_t (*system_func)(RCs...)) {
	InfoConstructor<RCs...> a(r_info);
}

/// `DataFetcher` is used to fetch the data from the world and provide it to the
/// `System`.
template <class C>
struct DataFetcher {};

/// Storage
template <class C>
struct DataFetcher<Storage<C> *> {
	Storage<C> *inner = nullptr;

	DataFetcher(World *p_world) {}

	void initiate_process(World *p_world) {
		inner = p_world->get_storage<C>();
	}

	void conclude_process(World *p_world) {}

	void set_active(bool p_active) {}
};

/// Spawner
template <class I>
struct DataFetcher<Spawner<I> &> {
	Spawner<I> inner;

	DataFetcher(World *p_world) {}

	void initiate_process(World *p_world) {
		inner.initiate_process(p_world);
	}

	void conclude_process(World *p_world) {}

	void set_active(bool p_active) {}
};

/// EventsEmitter
template <class E>
struct DataFetcher<EventsEmitter<E> &> {
	EventsEmitter<E> inner;

	DataFetcher(World *p_world) {}

	void initiate_process(World *p_world) {
		inner.initiate_process(p_world);
	}

	void conclude_process(World *p_world) {}

	void set_active(bool p_active) {}
};

/// Events
template <class E, typename EmitterName>
struct DataFetcher<EventsReceiver<E, EmitterName> &> {
	EventsReceiver<E, EmitterName> inner;

	DataFetcher(World *p_world) {}

	void initiate_process(World *p_world) {
		inner.initiate_process(p_world);
	}

	void conclude_process(World *p_world) {}

	void set_active(bool p_active) {}
};

/// Query
template <class... Cs>
struct DataFetcher<Query<Cs...> &> {
	Query<Cs...> inner;

	DataFetcher(World *p_world) :
			inner(p_world) {}

	void initiate_process(World *p_world) {
		inner.initiate_process(p_world);
	}

	void conclude_process(World *p_world) {
		inner.conclude_process(p_world);
	}

	void set_active(bool p_active) {
		inner.set_world_notification_active(p_active);
	}
};

/// Databag
template <class D>
struct DataFetcher<D *> {
	D *inner = nullptr;

	DataFetcher(World *p_world) {}

	void initiate_process(World *p_world) {
		inner = p_world->get_databag<D>();
	}

	void conclude_process(World *p_world) {}

	void set_active(bool p_active) {}
};

// ~~~~ system_execution_data definition ~~~~ //
#include "system_structs.gen.h"

// ~~~~ system_execution_data creation ~~~~ //
template <class R, class... Args>
uint64_t system_data_size_of(R (*func)(Args...)) {
	return system_data_size_of<Args...>();
}

template <class R, class... Args>
void system_data_new_placement(uint8_t *p_mem, World *p_world, R (*func)(Args...)) {
	system_data_new_placement<Args...>(p_mem, p_world);
}

template <class R, class... Args>
void system_data_delete_placement(uint8_t *p_mem, R (*func)(Args...)) {
	system_data_delete_placement<Args...>(p_mem);
}

template <class R, class... Args>
void system_data_set_active(uint8_t *p_mem, bool p_active, R (*func)(Args...)) {
	system_data_set_world_notificatin_active<Args...>(p_mem, p_active);
}

// ~~~~ system_exec_func definition ~~~~ //
#include "system_exe_funcs.gen.h"

// ~~~~ temporary_system_exec_func definition ~~~~ //
#include "system_dispatcher_exe_funcs.gen.h"

// ~~~~ temporary_system_exec_func definition ~~~~ //
#include "temporary_system_exe_funcs.gen.h"
} // namespace SystemBuilder
