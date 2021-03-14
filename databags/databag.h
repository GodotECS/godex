#pragma once

#include "../ecs_types.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/templates/local_vector.h"
#include "core/templates/oa_hash_map.h"

// This is necessary to fix Windows build: the macro `ECSCLASS` has the following
// line `friend class ECS`. Since the `Databag` is defined under the `godex`
// namespace, it's necessary specify `class ECS;` to make sure the compiler
// is able to properly resolve the `ECS` class.
class ECS;

namespace godex {

#define DATABAG(m_class)                                             \
	ECSCLASS(m_class)                                                \
                                                                     \
private:                                                             \
	/* Creation */                                                   \
	static _FORCE_INLINE_ m_class *create_databag() {                \
		return memnew(m_class);                                      \
	}                                                                \
	static _FORCE_INLINE_ godex::Databag *create_databag_no_type() { \
		/* Creates a storage but returns a generic component. */     \
		return create_databag();                                     \
	}                                                                \
                                                                     \
	/* Databag */                                                    \
	static inline uint32_t databag_id = UINT32_MAX;                  \
                                                                     \
public:                                                              \
	static uint32_t get_databag_id() { return databag_id; }          \
                                                                     \
	ECS_PROPERTY_MAPPER(m_class)                                     \
	ECS_METHOD_MAPPER(m_class)                                       \
                                                                     \
	static void __static_destructor() {                              \
		property_map.reset();                                        \
		properties.reset();                                          \
		setters.reset();                                             \
		getters.reset();                                             \
		methods_map.reset();                                         \
		methods.reset();                                             \
	}                                                                \
                                                                     \
public:

class Databag {
	ECSCLASS(Databag)
public:
	static void _bind_methods() {}
};

template <class T>
T *unwrap_databag(Object *p_access_databag) {
	DataAccessor *bag = dynamic_cast<DataAccessor *>(p_access_databag);
	if (unlikely(bag == nullptr || bag->get_target() == nullptr)) {
		return nullptr;
	}
	if (likely(bag->get_target_identifier() == T::get_databag_id() && bag->get_target_type() == DataAccessorTargetType::Databag)) {
		return static_cast<T *>(bag->get_target());
	} else {
		return nullptr;
	}
}

template <class T>
const T *unwrap_databag(const Object *p_access_databag) {
	const DataAccessor *bag = dynamic_cast<const DataAccessor *>(p_access_databag);
	if (unlikely(bag == nullptr || bag->get_target() == nullptr)) {
		return nullptr;
	}
	if (likely(bag->get_target_identifier() == T::get_databag_id() && bag->get_target_type() == DataAccessorTargetType::Databag)) {
		return static_cast<const T *>(bag->get_target());
	} else {
		return nullptr;
	}
}
} // namespace godex
