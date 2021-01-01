#pragma once

/* Author: AndreaCatania */

#include "../ecs.h"
#include "../storages/dense_vector.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/templates/oa_hash_map.h"

namespace godex {

#define COMPONENT(m_class, m_storage_class)                                                           \
	ECSCLASS(m_class)                                                                                 \
	friend class World;                                                                               \
	friend class Component;                                                                           \
																									  \
private:                                                                                              \
	/* Storages */                                                                                    \
	static _FORCE_INLINE_ m_storage_class<m_class> *create_storage() {                                \
		return memnew(m_storage_class<m_class>);                                                      \
	}                                                                                                 \
	static _FORCE_INLINE_ Storage *create_storage_no_type() {                                         \
		/* Creates a storage but returns a generic component. */                                      \
		return create_storage();                                                                      \
	}                                                                                                 \
																									  \
	/* Components */                                                                                  \
	static inline uint32_t component_id = UINT32_MAX;                                                 \
																									  \
	static void add_component_by_name(World *p_world, EntityID entity_id, const Dictionary &p_data) { \
		m_class component;                                                                            \
		for (const Variant *key = p_data.next(nullptr); key != nullptr; key = p_data.next(key)) {     \
			component.set(StringName(*key), p_data.get_valid(*key));                                  \
		}                                                                                             \
		p_world->add_component(                                                                       \
				entity_id,                                                                            \
				component);                                                                           \
	}                                                                                                 \
																									  \
public:                                                                                               \
	static uint32_t get_component_id() { return component_id; }                                       \
	virtual godex::component_id cid() const override { return component_id; }                         \
																									  \
	ECS_PROPERTY_MAPPER(m_class)                                                                      \
private:

class Component : public ECSClass {
	ECSCLASS(Component)

public:
	Component();

public:
	static void _bind_properties();

	/// Returns the component ID.
	virtual component_id cid() const;

	virtual const LocalVector<PropertyInfo> *get_properties() const;
	virtual bool set(const StringName &p_name, const Variant &p_data);
	virtual bool get(const StringName &p_name, Variant &r_data) const;

	virtual bool set(const uint32_t p_parameter_index, const Variant &p_data);
	virtual bool get(const uint32_t p_parameter_index, Variant &r_data) const;

	Variant get(const StringName &p_name) const;
};

template <class T>
T *unwrap_component(Object *p_access_resource) {
	DataAccessorScriptInstance<Component> *comp = dynamic_cast<DataAccessorScriptInstance<Component> *>(p_access_resource->get_script_instance());
	if (unlikely(comp == nullptr || comp->__target == nullptr)) {
		return nullptr;
	}
	if (likely(comp->__target->cid() == T::get_component_id())) {
		return static_cast<T *>(comp->__target);
	} else {
		return nullptr;
	}
}

template <class T>
const T *unwrap_component(const Object *p_access_resource) {
	const DataAccessorScriptInstance<Component> *comp = dynamic_cast<DataAccessorScriptInstance<Component> *>(p_access_resource->get_script_instance());
	if (unlikely(comp == nullptr || comp->__target == nullptr)) {
		return nullptr;
	}
	if (likely(comp->__target->cid() == T::get_component_id())) {
		return static_cast<const T *>(comp->__target);
	} else {
		return nullptr;
	}
}
} // namespace godex
