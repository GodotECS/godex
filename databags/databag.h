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
		static_property_map.reset();                                 \
		static_properties.reset();                                   \
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
} // namespace godex
