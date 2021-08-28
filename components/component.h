#pragma once

#include "../ecs.h"
#include "../storage/batch_storage.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/templates/oa_hash_map.h"

namespace godex {

#define COMPONENT_INTERNAL(m_class)                                                    \
	/* Components */                                                                   \
	static inline uint32_t component_id = UINT32_MAX;                                  \
                                                                                       \
public:                                                                                \
	static void *new_component() { return new m_class; }                               \
	static void free_component(void *p_component) { delete ((m_class *)p_component); } \
	static uint32_t get_component_id() { return component_id; }                        \
	ECS_PROPERTY_MAPPER(m_class)                                                       \
	ECS_METHOD_MAPPER(m_class)                                                         \
	static void __static_destructor() {                                                \
		static_property_map.reset();                                                   \
		static_properties.reset();                                                     \
		setters.reset();                                                               \
		getters.reset();                                                               \
		methods_map.reset();                                                           \
		methods.reset();                                                               \
	}                                                                                  \
                                                                                       \
public:

/// Register a component and allow to use a custom constructor.
#define COMPONENT_CUSTOM_CONSTRUCTOR(m_class, m_storage_class)         \
	ECSCLASS(m_class)                                                  \
	friend class World;                                                \
                                                                       \
public:                                                                \
	/* Storages */                                                     \
	static _FORCE_INLINE_ m_storage_class<m_class> *create_storage() { \
		return new m_storage_class<m_class>;                           \
	}                                                                  \
	static _FORCE_INLINE_ StorageBase *create_storage_no_type() {      \
		/* Creates a storage but returns a generic component. */       \
		return create_storage();                                       \
	}                                                                  \
	COMPONENT_INTERNAL(m_class)

/// Register a component.
#define COMPONENT(m_class, m_storage_class)                \
	COMPONENT_CUSTOM_CONSTRUCTOR(m_class, m_storage_class) \
	m_class() = default;                                   \
	m_class(const m_class &) = default;

/// Register a component using custom create storage function. The function is
/// specified on `ECS::register_component<Component>([]() -> StorageBase * { /* Create the storage and return it. */ });`.
#define COMPONENT_CUSTOM_STORAGE(m_class) \
	ECSCLASS(m_class)                     \
	friend class World;                   \
                                          \
private:                                  \
	COMPONENT_INTERNAL(m_class)           \
	m_class() = default;

/// Register a component that can store batched data.
#define COMPONENT_BATCH(m_class, m_storage_class, m_batch)                                    \
	ECSCLASS(m_class)                                                                         \
	friend class World;                                                                       \
                                                                                              \
private:                                                                                      \
	/* Storages */                                                                            \
	static _FORCE_INLINE_ BatchStorage<m_storage_class, m_batch, m_class> *create_storage() { \
		return new BatchStorage<m_storage_class, m_batch, m_class>;                           \
	}                                                                                         \
	static _FORCE_INLINE_ StorageBase *create_storage_no_type() {                             \
		/* Creates a storage but returns a generic component. */                              \
		return create_storage();                                                              \
	}                                                                                         \
	COMPONENT_INTERNAL(m_class)                                                               \
	m_class() = default;
} // namespace godex
