#pragma once

/* Author: AndreaCatania */

#include "core/object/object.h"
#include "core/string/ustring.h"

#define ECSCLASS(m_class)                             \
private:                                              \
	friend class ECS;                                 \
													  \
public:                                               \
	virtual String get_class() const override {       \
		return String(#m_class);                      \
	}                                                 \
	static _FORCE_INLINE_ String get_class_static() { \
		return String(#m_class);                      \
	}                                                 \
													  \
private:

class ECSClass {
public:
	virtual ~ECSClass() {}
	virtual String get_class() const {
		return "ECSClass";
	}
};

class EntityID {
	uint32_t id = UINT32_MAX;

public:
	EntityID() :
			id(UINT32_MAX) {}

	EntityID(uint32_t p_index) :
			id(p_index) {}

	bool is_null() const {
		return id == UINT32_MAX;
	}

	bool operator==(const EntityID &p_other) const {
		return id == p_other.id;
	}

	bool operator==(uint32_t p_naked_index) const {
		return id == p_naked_index;
	}

	operator uint32_t() const {
		return id;
	}
};

struct ScriptProperty {
	PropertyInfo property;
	Variant default_value;
};

namespace godex {
typedef uint32_t component_id;
typedef uint32_t resource_id;
typedef uint32_t system_id;
} // namespace godex

// ~~ PROPERTY MAPPER ~~

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

#define ECS_PROPERTY_MAPPER(m_class)                                                                                                   \
private:                                                                                                                               \
	/* Properties */                                                                                                                   \
	static inline OAHashMap<StringName, uint32_t> property_map;                                                                        \
	static inline LocalVector<PropertyInfo> properties;                                                                                \
	static inline LocalVector<godex::SetMethodHandleBase *> set_map;                                                                   \
	static inline LocalVector<godex::GetMethodHandleBase *> get_map;                                                                   \
	template <class M1, class M2>                                                                                                      \
	static void add_property(const PropertyInfo &p_info, M1 p_set, M2 p_get) {                                                         \
		const uint32_t index = properties.size();                                                                                      \
		property_map.insert(p_info.name, index);                                                                                       \
		properties.push_back(p_info);                                                                                                  \
																																	   \
		godex::SetMethodHandle<m_class, M1> *handle_set = new godex::SetMethodHandle<m_class, M1>;                                     \
		handle_set->method = p_set;                                                                                                    \
		set_map.push_back(handle_set);                                                                                                 \
																																	   \
		godex::GetMethodHandle<m_class, M2> *handle_get = new godex::GetMethodHandle<m_class, M2>;                                     \
		handle_get->method = p_get;                                                                                                    \
		get_map.push_back(handle_get);                                                                                                 \
	}                                                                                                                                  \
	static LocalVector<PropertyInfo> *get_properties_static() {                                                                        \
		return &properties;                                                                                                            \
	}                                                                                                                                  \
	static Variant get_property_default_static(StringName p_name) {                                                                    \
		const m_class c;                                                                                                               \
		Variant ret;                                                                                                                   \
		c.get(p_name, ret);                                                                                                            \
		return ret;                                                                                                                    \
	}                                                                                                                                  \
	static void clear_properties_static() {                                                                                            \
		property_map.clear();                                                                                                          \
	}                                                                                                                                  \
	virtual const LocalVector<PropertyInfo> *get_properties() const override {                                                         \
		return get_properties_static();                                                                                                \
	}                                                                                                                                  \
																																	   \
public:                                                                                                                                \
	virtual bool set(const StringName &p_name, const Variant &p_data) override {                                                       \
		const uint32_t *i_ptr = property_map.lookup_ptr(p_name);                                                                       \
		ERR_FAIL_COND_V_MSG(i_ptr == nullptr, false, "The parameter " + p_name + " doesn't exist in this component.");                 \
		return set(*i_ptr, p_data);                                                                                                    \
	}                                                                                                                                  \
	virtual bool get(const StringName &p_name, Variant &r_data) const override {                                                       \
		const uint32_t *i_ptr = property_map.lookup_ptr(p_name);                                                                       \
		ERR_FAIL_COND_V_MSG(i_ptr == nullptr, false, "The parameter " + p_name + " doesn't exist in this component.");                 \
		return get(*i_ptr, r_data);                                                                                                    \
	}                                                                                                                                  \
	virtual bool set(const uint32_t p_index, const Variant &p_data) override {                                                         \
		ERR_FAIL_COND_V_MSG(p_index >= set_map.size(), false, "The parameter " + itos(p_index) + " doesn't exist in this component."); \
		set_map[p_index]->set(this, p_data);                                                                                           \
		return true;                                                                                                                   \
	}                                                                                                                                  \
	virtual bool get(const uint32_t p_index, Variant &r_data) const override {                                                         \
		ERR_FAIL_COND_V_MSG(p_index >= get_map.size(), false, "The parameter " + itos(p_index) + " doesn't exist in this component."); \
		r_data = get_map[p_index]->get(this);                                                                                          \
		return true;                                                                                                                   \
	}

} // namespace godex

#define SINGLETON_MAKER(m_class)                                                                      \
private:                                                                                              \
	static inline m_class *singleton = nullptr;                                                       \
	m_class() {}                                                                                      \
																									  \
public:                                                                                               \
	static m_class *get_singleton() {                                                                 \
		if (unlikely(singleton == nullptr)) {                                                         \
			singleton = memnew(m_class);                                                              \
			Engine::get_singleton()->add_singleton(Engine::Singleton(get_class_static(), singleton)); \
		}                                                                                             \
		return singleton;                                                                             \
	}
