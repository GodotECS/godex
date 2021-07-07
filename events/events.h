
#pragma once

#include "../ecs.h"
#include "../storage/event_storage.h"
#include "../utils/typestring.hh"

#define EMITTER(str) typestring_is(#str)

namespace godex {
#define EVENT(m_class) \
	EVENT_CUSTOM_FLUSH(m_class, EVENT_CLEAR_MODE_FLUSH_ON_EMIT)

#define EVENT_CUSTOM_FLUSH(m_class, m_clear_mode)                                     \
	ECSCLASS(m_class)                                                                 \
	friend class World;                                                               \
                                                                                      \
	/* Event */                                                                       \
	static inline uint32_t event_id = UINT32_MAX;                                     \
                                                                                      \
public:                                                                               \
	static uint32_t get_event_id() { return event_id; }                               \
	static EventClearMode get_clear_mode() { return m_clear_mode; }                   \
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
	World *world;

public:
	EventsEmitter(World *p_world) :
			world(p_world) {}

	void emit(const String &p_emitter_name, const E &) {
		// TODO store the event in the world
		//world->
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
	World *world;

public:
	Events(World *p_world) :
			world(p_world) {}

	String get_emitter_name() const {
		return String(EmitterName::data());
	}
};
