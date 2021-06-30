
#pragma once

#include "../ecs.h"
#include "../storage/event_storage.h"

namespace godex {
#define EVENT(m_class, m_clear_mode)                                   \
	ECSCLASS(m_class)                                                  \
	friend class World;                                                \
                                                                       \
	/* Event */                                                        \
	static inline uint32_t event_id = UINT32_MAX;                      \
                                                                       \
public:                                                                \
	static uint32_t get_component_id() { return event_id; }            \
	static EventClearMode get_clear_mode() { return m_clear_mode; }    \
	ECS_PROPERTY_MAPPER(m_class)                                       \
	ECS_METHOD_MAPPER(m_class)                                         \
	static void __static_destructor() {                                \
		property_map.reset();                                          \
		properties.reset();                                            \
		setters.reset();                                               \
		getters.reset();                                               \
		methods_map.reset();                                           \
		methods.reset();                                               \
	}                                                                  \
                                                                       \
private:                                                               \
	/* Storages */                                                     \
	static _FORCE_INLINE_ EventStorage<m_class> *create_storage() {    \
		return new EventStorage<m_class>;                              \
	}                                                                  \
	static _FORCE_INLINE_ EventStorageBase *create_storage_no_type() { \
		/* Creates a storage but returns a generic component. */       \
		return create_storage();                                       \
	}                                                                  \
                                                                       \
public:                                                                \
	m_class() = default;
}; // namespace godex