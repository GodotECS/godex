
#pragma once

#include "../ecs.h"
#include "../storage/event_storage.h"
#include "../utils/typestring.hh"
#include "../world/world.h"

#define EMITTER(str) typestring_is(#str)

namespace godex {
#define EVENT(m_class)                                                                \
	ECSCLASS(m_class)                                                                 \
	friend class World;                                                               \
                                                                                      \
	/* Event */                                                                       \
	static inline uint32_t event_id = UINT32_MAX;                                     \
                                                                                      \
public:                                                                               \
	static uint32_t get_event_id() { return event_id; }                               \
	ECS_PROPERTY_MAPPER(m_class)                                                      \
	ECS_METHOD_MAPPER(m_class)                                                        \
	static void __static_destructor() {                                               \
		property_map.reset();                                                         \
		properties.reset();                                                           \
		setters.reset();                                                              \
		getters.reset();                                                              \
		methods_map.reset();                                                          \
		methods.reset();                                                              \
	}                                                                                 \
                                                                                      \
private:                                                                              \
	/* Storages */                                                                    \
	static _FORCE_INLINE_ EventStorage<m_class> *create_storage() {                   \
		return new EventStorage<m_class>;                                             \
	}                                                                                 \
	static _FORCE_INLINE_ void destroy_storage(EventStorage<m_class> *p_storage) {    \
		delete p_storage;                                                             \
	}                                                                                 \
	static _FORCE_INLINE_ EventStorageBase *create_storage_no_type() {                \
		/* Creates a storage but returns a generic component. */                      \
		return create_storage();                                                      \
	}                                                                                 \
	static _FORCE_INLINE_ void destroy_storage_no_type(EventStorageBase *p_storage) { \
		/* Creates a storage but returns a generic component. */                      \
		destroy_storage((EventStorage<m_class> *)p_storage);                          \
	}                                                                                 \
                                                                                      \
public:                                                                               \
	m_class() = default;
}; // namespace godex

/// Utility to emit an ECS event. This can be used by c++ systems:
/// ```
/// void my_emitter_system(EventsEmitter<MyEvent> &p_emitter){
///		p_emitter.emit("EmitterName1", MyEvent());
///		p_emitter.emit("EmitterName2", MyEvent());
/// }
/// ```
template <class E>
class EventsEmitter {
	EventStorage<E> *storage = nullptr;

public:
	EventsEmitter(World *p_world) {
		storage = p_world->get_events_storage<E>();
		// Flush the old events.
		storage->flush_events();
	}

	void emit(const String &p_emitter_name, const E &p_event) {
		storage->add_event(p_emitter_name, p_event);
	}
};

/// Utility that allow to fetch the events from the world. You can even use it
/// in a C++ system like this:
/// ```
/// void my_emitter_system(Events<MyEvent, EMITTER(EmitterName1)> &p_events){
///		for(const MyEvent& event : p_events) {
///			event....;
///		}
/// }
/// ```
template <class E, typename EmitterName>
class Events {
	const LocalVector<E> *emitter_storage = nullptr;

public:
	struct Iterator {
	private:
		const LocalVector<E> *emitter_storage = nullptr;

	public:
		using iterator_category = std::forward_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = const E *;
		uint32_t index = 0;

		Iterator(const LocalVector<E> *p_emitter_storage, uint32_t p_index) :
				emitter_storage(p_emitter_storage),
				index(p_index) {}

		bool is_valid() const {
			return index < emitter_storage->size();
		}

		value_type operator*() const {
			return emitter_storage->ptr() + index;
		}

		Iterator &operator++() {
			index += 1;
			return *this;
		}

		Iterator operator++(int) {
			Iterator tmp = *this;
			++(*this);
			return tmp;
		}

		friend bool operator==(const Iterator &a, const Iterator &b) { return a.index == b.index; }
		friend bool operator!=(const Iterator &a, const Iterator &b) { return a.index != b.index; }
	};

public:
	Events(World *p_world) {
		EventStorage<E> *storage = p_world->get_events_storage<E>();
		if (storage != nullptr) {
			emitter_storage = storage->get_events(get_emitter_name());
		}
	}

	static String get_emitter_name() {
		return String(EmitterName::data());
	}

	/// Returns the forward iterator to fetch the events.
	Iterator begin() {
		ERR_FAIL_COND_V_MSG(emitter_storage == nullptr, Iterator(emitter_storage, 0), "The emitter `" + E::get_class_static() + "::" + get_emitter_name() + "` storage doesn't exist.");
		return Iterator(emitter_storage, 0);
	}

	/// Used to know the last element of the `Iterator`.
	Iterator end() {
		ERR_FAIL_COND_V_MSG(emitter_storage == nullptr, Iterator(emitter_storage, 0), "The emitter `" + E::get_class_static() + "::" + get_emitter_name() + "` storage doesn't exist.");
		return Iterator(emitter_storage, emitter_storage->size());
	}
};
