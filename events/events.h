#pragma once

#include "../ecs.h"
#include "../storage/event_storage.h"

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
		static_property_map.reset();                                                  \
		static_properties.reset();                                                    \
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
