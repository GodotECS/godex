#pragma once

/* Author: AndreaCatania */

#include "../ecs_types.h"
#include "core/object/object.h"
#include "core/templates/oa_hash_map.h"

#define RESOURCE(m_class)                                                                      \
	ECSCLASS(m_class)                                                                          \
	friend class World;                                                                        \
																							   \
private:                                                                                       \
	/* Creation */                                                                             \
	static _FORCE_INLINE_ m_class *create_resource() {                                         \
		return memnew(m_class);                                                                \
	}                                                                                          \
	static _FORCE_INLINE_ godex::Resource *create_resource_no_type() {                         \
		/* Creates a storage but returns a generic component. */                               \
		return create_resource();                                                              \
	}                                                                                          \
																							   \
	/* Resource */                                                                             \
	static inline uint32_t resource_id = UINT32_MAX;                                           \
																							   \
public:                                                                                        \
	static uint32_t get_resource_id() { return resource_id; }                                  \
																							   \
private:                                                                                       \
	static inline OAHashMap<StringName, PropertyInfo> property_map;                            \
	static void add_property(const PropertyInfo &p_info, StringName p_set, StringName p_get) { \
		print_line("TODO integrate set and get.");                                             \
		property_map.insert(p_info.name, p_info);                                              \
	}                                                                                          \
	static OAHashMap<StringName, PropertyInfo> *get_properties_static() {                      \
		return &property_map;                                                                  \
	}                                                                                          \
	static void clear_properties_static() {                                                    \
		property_map.clear();                                                                  \
	}                                                                                          \
	virtual OAHashMap<StringName, PropertyInfo> *get_properties() const override {             \
		return get_properties_static();                                                        \
	}                                                                                          \
																							   \
private:

namespace godex {

class Resource : public ECSClass {
	ECSCLASS(Resource)

public:
	Resource();

public:
	static void _bind_properties();
	virtual OAHashMap<StringName, PropertyInfo> *get_properties() const;
};

} // namespace godex
