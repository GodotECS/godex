#pragma once

/* Author: AndreaCatania */

#include "core/object/object.h"
#include "core/templates/oa_hash_map.h"
#include "modules/ecs/ecs_types.h"

#define RESOURCE(m_class)                                                                      \
	ECSCLASS(m_class)                                                                          \
	friend class World;                                                                        \
																							   \
private:                                                                                       \
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

class ECSResource : public ECSClass {
	ECSCLASS(ECSResource)

public:
	ECSResource();

public:
	static void _bind_properties();
	virtual OAHashMap<StringName, PropertyInfo> *get_properties() const;
};
