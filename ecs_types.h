#pragma once

/* Author: AndreaCatania */

#include "core/object/object.h"
#include "core/object/script_language.h"
#include "core/string/ustring.h"
#include "core/templates/list.h"
#include "core/templates/local_vector.h"
#include "core/variant/binder_common.h"
#include "modules/gdscript/gdscript.h"

template <bool B>
struct bool_type {};

template <typename Test, template <typename...> class Ref>
struct is_specialization : bool_type<false> {};

template <template <typename...> class Ref, typename... Args>
struct is_specialization<Ref<Args...>, Ref> : bool_type<true> {};

#define ECSCLASS(m_class)                             \
private:                                              \
	friend class ECS;                                 \
													  \
public:                                               \
	static _FORCE_INLINE_ String get_class_static() { \
		return String(#m_class);                      \
	}                                                 \
													  \
private:

enum class StorageType {
	/// No storage.
	NONE,

	/// Store the data in a condenzed form factor.
	DENSE_VECTOR,

	/// Dynamically sized batch dense vector.
	/// Allow to store more components per entity. Since the batch is dynamically
	/// allocated, the cache coherency may not be respected.
	/// This is usually used by `events`.
	BATCH_DENSE_VECTOR,
};

/// `BatchData` it's used by the queries to return multiple `Component`s for
/// entity. Depending on the storage used, it's possible to store more
/// components per entity; in all these cases a `BatchData` is returned.
template <class C>
class Batch {
	C *data = nullptr;
	uint32_t size = 0;

public:
	Batch() = default;
	Batch(const Batch &) = default;

	Batch(C *p_data) :
			data(p_data), size(1) {}

	Batch(C *p_data, uint32_t p_size) :
			data(p_data), size(p_size) {}

	C *operator->() {
		return data;
	}

	const std::remove_const_t<C> *operator->() const {
		return data;
	}

	C *operator[](uint32_t p_index) {
#ifdef DEBUG_ENABLED
		CRASH_COND(p_index >= size);
#endif
		return data + p_index;
	}

	const std::remove_const_t<C> *operator[](uint32_t p_index) const {
#ifdef DEBUG_ENABLED
		CRASH_COND(p_index >= size);
#endif
		return data + p_index;
	}

	uint32_t get_size() const {
		return size;
	}

	bool is_batch() const {
		return size > 1;
	}

	bool is_empty() const {
		return data == nullptr;
	}

	bool operator==(const Batch<C> &p_other) const {
		return data == p_other.data && size == p_other.size;
	}

	bool operator!=(const Batch<C> &p_other) const {
		return data != p_other.data && size != p_other.size;
	}

	bool operator==(const void *p_other) const {
		return data == p_other;
	}

	bool operator!=(const void *p_other) const {
		return data != p_other;
	}

	C *get_data() {
		return data;
	}

	C *get_data() const {
		return data;
	}

	operator C *() {
		return data;
	}
};

class EntityID {
	uint32_t id = UINT32_MAX;

public:
	EntityID() :
			id(UINT32_MAX) {}

	EntityID(const EntityID &) = default;

	EntityID(uint32_t p_index) :
			id(p_index) {}

	EntityID(Variant p_index) :
			id(p_index.operator unsigned int()) {}

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

	operator Variant() const {
		return Variant(id);
	}
};

struct ScriptProperty {
	PropertyInfo property;
	Variant default_value;
};

namespace godex {
typedef uint32_t component_id;
typedef uint32_t databag_id;
typedef uint32_t system_id;
} // namespace godex

// ~~ PROPERTY MAPPER ~~

namespace godex {

typedef bool (*func_setter)(void *, const Variant &p_data);
typedef bool (*func_getter)(const void *, Variant &r_data);

#define ECS_PROPERTY_MAPPER(m_class)                                                                                                   \
private:                                                                                                                               \
	/* Properties */                                                                                                                   \
	static inline LocalVector<StringName> property_map;                                                                                \
	static inline LocalVector<PropertyInfo> properties;                                                                                \
	static inline LocalVector<godex::func_setter> setters;                                                                             \
	static inline LocalVector<godex::func_getter> getters;                                                                             \
	static void add_property(const PropertyInfo &p_info, godex::func_setter p_set, godex::func_getter p_get) {                         \
		property_map.push_back(p_info.name);                                                                                           \
		properties.push_back(p_info);                                                                                                  \
		setters.push_back(p_set);                                                                                                      \
		getters.push_back(p_get);                                                                                                      \
	}                                                                                                                                  \
	static const LocalVector<PropertyInfo> *get_properties() {                                                                         \
		return &properties;                                                                                                            \
	}                                                                                                                                  \
	static Variant get_property_default(const StringName &p_name) {                                                                    \
		const m_class c;                                                                                                               \
		Variant ret;                                                                                                                   \
		get_by_name(&c, p_name, ret);                                                                                                  \
		return ret;                                                                                                                    \
	}                                                                                                                                  \
	static void clear_properties_static() {                                                                                            \
		property_map.clear();                                                                                                          \
	}                                                                                                                                  \
	static uint32_t get_property_index(const StringName &p_name) {                                                                     \
		const int64_t i = property_map.find(p_name);                                                                                   \
		return i == -1 ? UINT32_MAX : uint32_t(i);                                                                                     \
	}                                                                                                                                  \
																																	   \
public:                                                                                                                                \
	static bool set_by_name(void *p_self, const StringName &p_name, const Variant &p_data) {                                           \
		m_class *self = static_cast<m_class *>(p_self);                                                                                \
		const uint32_t i = get_property_index(p_name);                                                                                 \
		ERR_FAIL_COND_V_MSG(i == UINT32_MAX, false, "The parameter " + p_name + " doesn't exist in this component.");                  \
		return setters[i](self, p_data);                                                                                               \
	}                                                                                                                                  \
	static bool get_by_name(const void *p_self, const StringName &p_name, Variant &r_data) {                                           \
		const m_class *self = static_cast<const m_class *>(p_self);                                                                    \
		const uint32_t i = get_property_index(p_name);                                                                                 \
		ERR_FAIL_COND_V_MSG(i == UINT32_MAX, false, "The parameter " + p_name + " doesn't exist in this component.");                  \
		return getters[i](self, r_data);                                                                                               \
	}                                                                                                                                  \
	static bool set_by_index(void *p_self, const uint32_t p_index, const Variant &p_data) {                                            \
		m_class *self = static_cast<m_class *>(p_self);                                                                                \
		ERR_FAIL_COND_V_MSG(p_index >= setters.size(), false, "The parameter " + itos(p_index) + " doesn't exist in this component."); \
		return setters[p_index](self, p_data);                                                                                         \
	}                                                                                                                                  \
	static bool get_by_index(const void *p_self, const uint32_t p_index, Variant &r_data) {                                            \
		const m_class *self = static_cast<const m_class *>(p_self);                                                                    \
		ERR_FAIL_COND_V_MSG(p_index >= getters.size(), false, "The parameter " + itos(p_index) + " doesn't exist in this component."); \
		return getters[p_index](self, r_data);                                                                                         \
	}

/// Must be called in `_bind_methods` and can be used to just bind a property.
#define ECS_BIND_PROPERTY(clazz, prop_info, prop)                \
	add_property(                                                \
			prop_info,                                           \
			[](void *self, const Variant &p_data) -> bool {      \
				static_cast<clazz *>(self)->prop = p_data;       \
				return true;                                     \
			},                                                   \
			[](const void *self, Variant &r_data) -> bool {      \
				r_data = static_cast<const clazz *>(self)->prop; \
				return true;                                     \
			});

/// Must be called in `_bind_methods` and can be used to bind set and get of a
/// property. Useful when you have to parse the set/get data.
#define ECS_BIND_PROPERTY_FUNC(clazz, prop_info, set_func, get_func)   \
	add_property(                                                      \
			prop_info,                                                 \
			[](void *self, const Variant &p_data) -> bool {            \
				static_cast<clazz *>(self)->set_func(p_data);          \
				return true;                                           \
			},                                                         \
			[](const void *self, Variant &r_data) -> bool {            \
				r_data = static_cast<const clazz *>(self)->get_func(); \
				return true;                                           \
			});
} // namespace godex

// ~~ METHOD MAPPER ~~

namespace godex {

struct MethodHelperBase {
	virtual int get_argument_count() const {
		return 0;
	}
	virtual ~MethodHelperBase() {}

	virtual void call(void *p_obj, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error) {}
};

template <class R, class C, class... Args, size_t... Is>
void call_return_mutable(C *p_obj, R (C::*method)(Args...), const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error, IndexSequence<Is...>) {
	*r_ret = (p_obj->*method)(VariantCaster<Args>::cast(*p_args[Is])...);
}

template <class R, class C, class... Args>
struct MethodHelperR : public MethodHelperBase {
	R(C::*method)
	(Args...);

	MethodHelperR(R (C::*p_method)(Args...)) :
			method(p_method) {}

	virtual ~MethodHelperR() {}

	virtual int get_argument_count() const override {
		return int(sizeof...(Args));
	}

	virtual void call(void *p_obj, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error) override {
		call_return_mutable(static_cast<C *>(p_obj), method, p_args, p_argcount, r_ret, r_error, BuildIndexSequence<sizeof...(Args)>{});
	}
};

template <class R, class C, class... Args, size_t... Is>
void call_return_immutable(const C *p_obj, R (C::*method)(Args...) const, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error, IndexSequence<Is...>) {
	*r_ret = (p_obj->*method)(VariantCaster<Args>::cast(*p_args[Is])...);
}

template <class R, class C, class... Args>
struct MethodHelperRC : public MethodHelperBase {
	R(C::*method)
	(Args...) const;

	MethodHelperRC(R (C::*p_method)(Args...) const) :
			method(p_method) {}

	virtual int get_argument_count() const override {
		return int(sizeof...(Args));
	}

	virtual void call(void *p_obj, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error) override {
		call_return_immutable(static_cast<const C *>(p_obj), method, p_args, p_argcount, r_ret, r_error, BuildIndexSequence<sizeof...(Args)>{});
	}
};

template <class C, class... Args, size_t... Is>
void call_rvoid_mutable(C *p_obj, void (C::*method)(Args...), const Variant **p_args, int p_argcount, Callable::CallError &r_error, IndexSequence<Is...>) {
	(p_obj->*method)(VariantCaster<Args>::cast(*p_args[Is])...);
}

template <class C, class... Args>
struct MethodHelper : public MethodHelperBase {
	void (C::*method)(Args...);

	MethodHelper(void (C::*p_method)(Args...)) :
			method(p_method) {}

	virtual int get_argument_count() const override {
		return int(sizeof...(Args));
	}

	virtual void call(void *p_obj, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error) override {
		call_rvoid_mutable(static_cast<C *>(p_obj), method, p_args, p_argcount, r_error, BuildIndexSequence<sizeof...(Args)>{});
	}
};

template <class C, class... Args, size_t... Is>
void call_rvoid_immutable(const C *p_obj, void (C::*method)(Args...) const, const Variant **p_args, int p_argcount, Callable::CallError &r_error, IndexSequence<Is...>) {
	(p_obj->*method)(VariantCaster<Args>::cast(*p_args[Is])...);
}

template <class C, class... Args>
struct MethodHelperC : public MethodHelperBase {
	void (C::*method)(Args...) const;

	MethodHelperC(void (C::*p_method)(Args...) const) :
			method(p_method) {}

	virtual int get_argument_count() const override {
		return int(sizeof...(Args));
	}

	virtual void call(void *p_obj, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error) override {
		call_rvoid_immutable(static_cast<const C *>(p_obj), method, p_args, p_argcount, r_error, BuildIndexSequence<sizeof...(Args)>{});
	}
};

#define ECS_METHOD_MAPPER(m_class)                                                                                                                                                    \
private:                                                                                                                                                                              \
	static inline LocalVector<StringName> methods_map;                                                                                                                                \
	static inline LocalVector<godex::MethodHelperBase *> methods;                                                                                                                     \
																																													  \
public:                                                                                                                                                                               \
	/* Adds methods with a return type. */                                                                                                                                            \
	template <class R, class C, class... Args>                                                                                                                                        \
	static void add_method(const StringName &p_method, R (C::*method)(Args...)) {                                                                                                     \
		ERR_FAIL_COND_MSG(methods_map.find(p_method) >= 0, "The method " + p_method + " is already registered: " + get_class_static());                                               \
		methods_map.push_back(p_method);                                                                                                                                              \
		methods.push_back(new godex::MethodHelperR<R, C, Args...>(method));                                                                                                           \
	};                                                                                                                                                                                \
																																													  \
	/* Adds methods with a return type and constants. */                                                                                                                              \
	template <class R, class C, class... Args>                                                                                                                                        \
	static void add_method(const StringName &p_method, R (C::*method)(Args...) const) {                                                                                               \
		ERR_FAIL_COND_MSG(methods_map.find(p_method) >= 0, "The method " + p_method + " is already registered: " + get_class_static());                                               \
		methods_map.push_back(p_method);                                                                                                                                              \
		methods.push_back(new godex::MethodHelperRC<R, C, Args...>(method));                                                                                                          \
	}                                                                                                                                                                                 \
																																													  \
	/* Adds methods without a return type. */                                                                                                                                         \
	template <class C, class... Args>                                                                                                                                                 \
	static void add_method(const StringName &p_method, void (C::*method)(Args...)) {                                                                                                  \
		ERR_FAIL_COND_MSG(methods_map.find(p_method) >= 0, "The method " + p_method + " is already registered: " + get_class_static());                                               \
		methods_map.push_back(p_method);                                                                                                                                              \
		methods.push_back(new godex::MethodHelper<C, Args...>(method));                                                                                                               \
	}                                                                                                                                                                                 \
																																													  \
	/* Adds methods without a return type and constants.*/                                                                                                                            \
	template <class C, class... Args>                                                                                                                                                 \
	static void add_method(const StringName &p_method, void (C::*method)(Args...) const) {                                                                                            \
		ERR_FAIL_COND_MSG(methods_map.find(p_method) >= 0, "The method " + p_method + " is already registered: " + get_class_static());                                               \
		methods_map.push_back(p_method);                                                                                                                                              \
		methods.push_back(new godex::MethodHelperC<C, Args...>(method));                                                                                                              \
	}                                                                                                                                                                                 \
																																													  \
	static void static_call(void *p_self, const StringName &p_method, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error) {                         \
		m_class *self = static_cast<m_class *>(p_self);                                                                                                                               \
		self->call(p_method, p_args, p_argcount, r_ret, r_error);                                                                                                                     \
	}                                                                                                                                                                                 \
																																													  \
	void call(const StringName &p_method, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error) {                                                     \
		const int64_t _index = methods_map.find(p_method);                                                                                                                            \
		if (unlikely(_index < 0)) {                                                                                                                                                   \
			ERR_PRINT("The method " + p_method + " is unknown " + get_class_static());                                                                                                \
			r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;                                                                                                           \
			return;                                                                                                                                                                   \
		}                                                                                                                                                                             \
		const uint32_t index = _index;                                                                                                                                                \
		if (unlikely(methods[index]->get_argument_count() != p_argcount)) {                                                                                                           \
			ERR_PRINT("The method " + p_method + " is has " + itos(methods[index]->get_argument_count()) + " arguments; provided: " + itos(p_argcount) + " - " + get_class_static()); \
			if (methods[index]->get_argument_count() > p_argcount) {                                                                                                                  \
				r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;                                                                                                    \
			} else {                                                                                                                                                                  \
				r_error.error = Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;                                                                                                   \
			}                                                                                                                                                                         \
			r_error.argument = p_argcount;                                                                                                                                            \
			r_error.expected = methods[index]->get_argument_count();                                                                                                                  \
			return;                                                                                                                                                                   \
		}                                                                                                                                                                             \
		methods[index]->call(this, p_args, p_argcount, r_ret, r_error);                                                                                                               \
	}                                                                                                                                                                                 \
																																													  \
private:
} // namespace godex

#define SINGLETON_MAKER(m_class)                                                                      \
private:                                                                                              \
	static inline m_class *singleton = nullptr;                                                       \
																									  \
public:                                                                                               \
	static m_class *get_singleton() {                                                                 \
		if (unlikely(singleton == nullptr)) {                                                         \
			singleton = memnew(m_class);                                                              \
			Engine::get_singleton()->add_singleton(Engine::Singleton(get_class_static(), singleton)); \
		}                                                                                             \
		return singleton;                                                                             \
	}

enum class DataAccessorTargetType {
	Databag,
	Component,
	Storage,
};

/// This is useful to access the Component / Databag / Storage.
class DataAccessor : public Object {
private:
	uint32_t target_identifier;
	DataAccessorTargetType target_type;
	bool mut = false;
	void *target = nullptr;

public:
	void init(uint32_t p_identifier, DataAccessorTargetType p_type, bool p_mut);

	uint32_t get_target_identifier() const;
	DataAccessorTargetType get_target_type() const;
	bool is_mutable() const;

	void set_target(void *p_target);
	void *get_target();
	const void *get_target() const;

	virtual bool _setv(const StringName &p_name, const Variant &p_value) override;
	virtual bool _getv(const StringName &p_name, Variant &r_ret) const override;
	virtual Variant call(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) override;
};
