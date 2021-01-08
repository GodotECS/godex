#pragma once

/* Author: AndreaCatania */

#include "../ecs_types.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/templates/local_vector.h"
#include "core/templates/oa_hash_map.h"

namespace godex {

#define DATABAG(m_class)                                                  \
	ECSCLASS(m_class)                                                     \
																		  \
private:                                                                  \
	/* Creation */                                                        \
	static _FORCE_INLINE_ m_class *create_databag() {                     \
		return memnew(m_class);                                           \
	}                                                                     \
	static _FORCE_INLINE_ godex::Databag *create_databag_no_type() {      \
		/* Creates a storage but returns a generic component. */          \
		return create_databag();                                          \
	}                                                                     \
																		  \
	/* Databag */                                                         \
	static inline uint32_t databag_id = UINT32_MAX;                       \
																		  \
public:                                                                   \
	static uint32_t get_databag_id() { return databag_id; }               \
	virtual godex::databag_id rid() const override { return databag_id; } \
																		  \
	ECS_PROPERTY_MAPPER(m_class)                                          \
private:

class Databag : public ECSClass {
	ECSCLASS(Databag)

public:
	Databag();

public:
	static void _bind_properties();

	/// Returns the resouce ID.
	virtual databag_id rid() const;

	virtual const LocalVector<PropertyInfo> *get_properties() const;
	virtual bool set(const StringName &p_name, const Variant &p_data);
	virtual bool get(const StringName &p_name, Variant &r_data) const;

	virtual bool set(const uint32_t p_parameter_index, const Variant &p_data);
	virtual bool get(const uint32_t p_parameter_index, Variant &r_data) const;

	Variant get(const StringName &p_name) const;
};

template <class T>
T *unwrap_databag(Object *p_access_databag) {
	DataAccessorScriptInstance<Databag> *res = dynamic_cast<DataAccessorScriptInstance<Databag> *>(p_access_databag->get_script_instance());
	if (unlikely(res == nullptr || res->__target == nullptr)) {
		return nullptr;
	}
	if (likely(res->__target->rid() == T::get_databag_id())) {
		return static_cast<T *>(res->__target);
	} else {
		return nullptr;
	}
}

template <class T>
const T *unwrap_databag(const Object *p_access_databag) {
	const DataAccessorScriptInstance<Databag> *res = dynamic_cast<DataAccessorScriptInstance<Databag> *>(p_access_databag->get_script_instance());
	if (unlikely(res == nullptr || res->__target == nullptr)) {
		return nullptr;
	}
	if (likely(res->__target->rid() == T::get_databag_id())) {
		return static_cast<const T *>(res->__target);
	} else {
		return nullptr;
	}
}

} // namespace godex
