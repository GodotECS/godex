#pragma once

/* Author: AndreaCatania */

#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/templates/local_vector.h"
#include "core/templates/oa_hash_map.h"
#include "modules/ecs/ecs.h"
#include "modules/ecs/storages/dense_vector.h"

namespace godex {

struct SetMethodHandleBase {
	virtual ~SetMethodHandleBase() {}
	virtual void set(void *p_object, const Variant &p_data) = 0;
};

template <class C, class T>
struct SetMethodHandle : public SetMethodHandleBase {
	T method;

	virtual void set(void *p_object, const Variant &p_data) override {
		(static_cast<C *>(p_object)->*method)(p_data);
	}
};

struct GetMethodHandleBase {
	virtual ~GetMethodHandleBase() {}
	virtual Variant get(const void *p_object) = 0;
};

template <class C, class T>
struct GetMethodHandle : public GetMethodHandleBase {
	T method;

	virtual Variant get(const void *p_object) override {
		return (static_cast<const C *>(p_object)->*method)();
	}
};

#define COMPONENT(m_class, m_storage_class)                                                                            \
	ECSCLASS(m_class)                                                                                                  \
	friend class World;                                                                                                \
	friend class Component;                                                                                            \
																													   \
private:                                                                                                               \
	/* Storages */                                                                                                     \
	static _FORCE_INLINE_ m_storage_class<m_class> *create_storage() {                                                 \
		return memnew(m_storage_class<m_class>);                                                                       \
	}                                                                                                                  \
	static _FORCE_INLINE_ Storage *create_storage_no_type() {                                                          \
		/* Creates a storage but returns a generic component. */                                                       \
		return create_storage();                                                                                       \
	}                                                                                                                  \
																													   \
	/* Components */                                                                                                   \
	static inline uint32_t component_id = UINT32_MAX;                                                                  \
																													   \
	static void add_component_by_name(World *p_world, EntityID entity_id, const Dictionary &p_data) {                  \
		m_class component;                                                                                             \
		for (const Variant *key = p_data.next(nullptr); key != nullptr; key = p_data.next(key)) {                      \
			component.set(*key, p_data.get_valid(*key));                                                               \
		}                                                                                                              \
		p_world->add_component(                                                                                        \
				entity_id,                                                                                             \
				component);                                                                                            \
	}                                                                                                                  \
																													   \
public:                                                                                                                \
	static uint32_t get_component_id() { return component_id; }                                                        \
																													   \
	/* Properties */                                                                                                   \
private:                                                                                                               \
	static inline OAHashMap<StringName, uint32_t> property_map;                                                        \
	static inline LocalVector<PropertyInfo> properties;                                                                \
	static inline LocalVector<godex::SetMethodHandleBase *> set_map;                                                   \
	static inline LocalVector<godex::GetMethodHandleBase *> get_map;                                                   \
	template <class M1, class M2>                                                                                      \
	static void add_property(const PropertyInfo &p_info, M1 p_set, M2 p_get) {                                         \
		const uint32_t index = properties.size();                                                                      \
		property_map.insert(p_info.name, index);                                                                       \
		properties.push_back(p_info);                                                                                  \
																													   \
		godex::SetMethodHandle<m_class, M1> *handle_set = new godex::SetMethodHandle<m_class, M1>;                     \
		handle_set->method = p_set;                                                                                    \
		set_map.push_back(handle_set);                                                                                 \
																													   \
		godex::GetMethodHandle<m_class, M2> *handle_get = new godex::GetMethodHandle<m_class, M2>;                     \
		handle_get->method = p_get;                                                                                    \
		get_map.push_back(handle_get);                                                                                 \
	}                                                                                                                  \
	static LocalVector<PropertyInfo> *get_properties_static() {                                                        \
		return &properties;                                                                                            \
	}                                                                                                                  \
	static Variant get_property_default_static(StringName p_name) {                                                    \
		const m_class c;                                                                                               \
		Variant ret;                                                                                                   \
		c.get(p_name, ret);                                                                                            \
		return ret;                                                                                                    \
	}                                                                                                                  \
	static void clear_properties_static() {                                                                            \
		property_map.clear();                                                                                          \
	}                                                                                                                  \
	virtual const LocalVector<PropertyInfo> *get_properties() const override {                                         \
		return get_properties_static();                                                                                \
	}                                                                                                                  \
																													   \
public:                                                                                                                \
	virtual bool set(const StringName &p_name, const Variant &p_data) override {                                       \
		const uint32_t *i_ptr = property_map.lookup_ptr(p_name);                                                       \
		ERR_FAIL_COND_V_MSG(i_ptr == nullptr, false, "The parameter " + p_name + " doesn't exist in this component."); \
		set_map[*i_ptr]->set(this, p_data);                                                                            \
		return true;                                                                                                   \
	}                                                                                                                  \
	virtual bool get(const StringName &p_name, Variant &r_data) const override {                                       \
		const uint32_t *i_ptr = property_map.lookup_ptr(p_name);                                                       \
		ERR_FAIL_COND_V_MSG(i_ptr == nullptr, false, "The parameter " + p_name + " doesn't exist in this component."); \
		r_data = get_map[*i_ptr]->get(this);                                                                           \
		return true;                                                                                                   \
	}                                                                                                                  \
																													   \
private:

class Component : public ECSClass {
	ECSCLASS(Component)

public:
	Component();

public:
	static void _bind_properties();

	virtual const LocalVector<PropertyInfo> *get_properties() const;
	virtual bool set(const StringName &p_name, const Variant &p_data);
	virtual bool get(const StringName &p_name, Variant &r_data) const;

	Variant get(const StringName &p_name) const;
};

} // namespace godex
