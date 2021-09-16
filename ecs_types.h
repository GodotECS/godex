#pragma once

#include "core/object/object.h"
#include "core/object/script_language.h"
#include "core/string/ustring.h"
#include "core/templates/list.h"
#include "core/templates/local_vector.h"
#include "core/variant/binder_common.h"
#include "modules/gdscript/gdscript.h"

struct SystemExeInfo;
class World;

template <typename T, typename = void>
struct godex_has_get_spawners : std::false_type {};

template <typename T>
struct godex_has_get_spawners<T, decltype(void(std::declval<T &>().get_spawners()))> : std::true_type {};

template <typename T, typename = void>
struct godex_has__bind_methods : std::false_type {};

template <typename T>
struct godex_has__bind_methods<T, decltype(void(std::declval<T &>()._bind_methods()))> : std::true_type {};

template <typename T, typename = void>
struct godex_has__get : std::false_type {};

template <typename T>
struct godex_has__get<T, decltype(void(std::declval<T &>()._get(std::declval<const StringName &>(), std::declval<Variant &>())))> : std::true_type {};

template <typename T, typename = void>
struct godex_has__set : std::false_type {};

template <typename T>
struct godex_has__set<T, decltype(void(std::declval<T &>()._set(std::declval<const StringName &>(), std::declval<const Variant &>())))> : std::true_type {};

template <typename T, typename = void>
struct godex_has__get_property_list : std::false_type {};

template <typename T>
struct godex_has__get_property_list<T, decltype(void(std::declval<T &>()._get_property_list(std::declval<List<PropertyInfo> *>())))> : std::true_type {};

template <typename T, typename = void>
struct godex_has_storage_config : std::false_type {};

template <typename T>
struct godex_has_storage_config<T, decltype(void(std::declval<T &>()._get_storage_config(std::declval<Dictionary &>())))> : std::true_type {};

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

	bool is_valid() const {
		return id != UINT32_MAX;
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
/// This is used to introduce a new editor hint: This hint is used to display
/// the list of available spawners on editor.
constexpr int PROPERTY_HINT_ECS_SPAWNER = PROPERTY_HINT_MAX + 1;
constexpr int PROPERTY_HINT_ECS_EVENT_EMITTER = PROPERTY_HINT_MAX + 2;
} // namespace godex

namespace godex {
typedef uint32_t spawner_id;
constexpr spawner_id SPAWNER_NONE = UINT32_MAX;

typedef uint32_t component_id;
constexpr component_id COMPONENT_NONE = UINT32_MAX;

typedef uint32_t databag_id;
constexpr databag_id DATABAG_NONE = UINT32_MAX;

typedef uint32_t event_id;
constexpr event_id EVENT_NONE = UINT32_MAX;

typedef uint32_t system_id;
constexpr system_id SYSTEM_NONE = UINT32_MAX;

typedef uint32_t system_bundle_id;
constexpr system_bundle_id SYSTEM_BUNDLE_NONE = UINT32_MAX;

/// Shared Component ID, used to identify a component.
typedef uint32_t SID;
constexpr SID SID_NONE = UINT32_MAX;
} // namespace godex

// ~~ PROPERTY MAPPER ~~

namespace godex {

template <class X>
bool global_dynamic_get([[maybe_unused]] X *p_self, [[maybe_unused]] const StringName &p_name, [[maybe_unused]] Variant &r_data) {
	if constexpr (godex_has__get<X>::value) {
		return p_self->_get(p_name, r_data);
	} else {
		return false;
	}
}

template <class X>
bool global_dynamic_set([[maybe_unused]] X *p_self, [[maybe_unused]] const StringName &p_name, [[maybe_unused]] const Variant &r_data) {
	if constexpr (godex_has__set<X>::value) {
		return p_self->_set(p_name, r_data);
	} else {
		return false;
	}
}

template <class X>
void global_dynamic_get_property_list([[maybe_unused]] X *p_self, List<PropertyInfo> *r_list) {
	const LocalVector<PropertyInfo> *static_properties = X::get_static_properties();
	for (uint32_t i = 0; i < static_properties->size(); ++i) {
		r_list->push_back((*static_properties)[i]);
	}
	if constexpr (godex_has__get_property_list<X>::value) {
		if (p_self) {
			p_self->_get_property_list(r_list);
		}
	}
}

typedef bool (*func_setter)(void *, const Variant &p_data);
typedef bool (*func_getter)(const void *, Variant &r_data);

template <class C>
C init_class() {
	return C();
}

#define ECS_PROPERTY_MAPPER(m_class)                                                                                                     \
private:                                                                                                                                 \
	/* Properties */                                                                                                                     \
	static inline LocalVector<StringName> static_property_map;                                                                           \
	static inline LocalVector<PropertyInfo> static_properties;                                                                           \
	static inline LocalVector<godex::func_setter> setters;                                                                               \
	static inline LocalVector<godex::func_getter> getters;                                                                               \
	static void add_property(const PropertyInfo &p_info, godex::func_setter p_set, godex::func_getter p_get) {                           \
		static_property_map.push_back(p_info.name);                                                                                      \
		static_properties.push_back(p_info);                                                                                             \
		setters.push_back(p_set);                                                                                                        \
		getters.push_back(p_get);                                                                                                        \
	}                                                                                                                                    \
	static void clear_properties_static() {                                                                                              \
		static_property_map.clear();                                                                                                     \
	}                                                                                                                                    \
                                                                                                                                         \
public:                                                                                                                                  \
	static const LocalVector<PropertyInfo> *get_static_properties() {                                                                    \
		return &static_properties;                                                                                                       \
	}                                                                                                                                    \
	static void get_property_list(void *p_self, List<PropertyInfo> *r_list) {                                                            \
		m_class *self = static_cast<m_class *>(p_self);                                                                                  \
		godex::global_dynamic_get_property_list(self, r_list);                                                                           \
	}                                                                                                                                    \
	static Variant get_property_default(const StringName &p_name) {                                                                      \
		const m_class c = godex::init_class<m_class>();                                                                                  \
		Variant ret;                                                                                                                     \
		get_by_name(&c, p_name, ret);                                                                                                    \
		return ret;                                                                                                                      \
	}                                                                                                                                    \
	static uint32_t get_property_index(const StringName &p_name) {                                                                       \
		const int64_t i = static_property_map.find(p_name);                                                                              \
		return i == -1 ? UINT32_MAX : uint32_t(i);                                                                                       \
	}                                                                                                                                    \
	static bool set_by_name(void *p_self, const StringName &p_name, const Variant &p_data) {                                             \
		m_class *self = static_cast<m_class *>(p_self);                                                                                  \
		const uint32_t i = get_property_index(p_name);                                                                                   \
		if (i == UINT32_MAX) {                                                                                                           \
			ERR_FAIL_COND_V_MSG(!godex::global_dynamic_set(self, p_name, p_data), false, "The parameter " + p_name + " doesn't exist."); \
			return true;                                                                                                                 \
		} else {                                                                                                                         \
			return setters[i](self, p_data);                                                                                             \
		}                                                                                                                                \
	}                                                                                                                                    \
	static bool get_by_name(const void *p_self, const StringName &p_name, Variant &r_data) {                                             \
		const m_class *self = static_cast<const m_class *>(p_self);                                                                      \
		const uint32_t i = get_property_index(p_name);                                                                                   \
		if (i == UINT32_MAX) {                                                                                                           \
			ERR_FAIL_COND_V_MSG(!godex::global_dynamic_get(self, p_name, r_data), false, "The parameter " + p_name + " doesn't exist."); \
			return true;                                                                                                                 \
		} else {                                                                                                                         \
			return getters[i](self, r_data);                                                                                             \
		}                                                                                                                                \
	}                                                                                                                                    \
	static bool set_by_index(void *p_self, const uint32_t p_index, const Variant &p_data) {                                              \
		m_class *self = static_cast<m_class *>(p_self);                                                                                  \
		ERR_FAIL_COND_V_MSG(p_index >= setters.size(), false, "The parameter " + itos(p_index) + " doesn't exist in this component.");   \
		return setters[p_index](self, p_data);                                                                                           \
	}                                                                                                                                    \
	static bool get_by_index(const void *p_self, const uint32_t p_index, Variant &r_data) {                                              \
		const m_class *self = static_cast<const m_class *>(p_self);                                                                      \
		ERR_FAIL_COND_V_MSG(p_index >= getters.size(), false, "The parameter " + itos(p_index) + " doesn't exist in this component.");   \
		return getters[p_index](self, r_data);                                                                                           \
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
		r_error.error = Callable::CallError::CALL_OK;                                                                                                                                 \
	}                                                                                                                                                                                 \
                                                                                                                                                                                      \
public:
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

class GodexWorldFetcher : public Object {
	GDCLASS(GodexWorldFetcher, Object)

public:
	virtual void get_system_info(SystemExeInfo *r_info) const = 0;

	virtual void prepare_world(World *p_world) = 0;
	virtual void initiate_process(World *p_world) = 0;
	virtual void conclude_process(World *p_world) = 0;
	virtual void release_world(World *p_world) = 0;
	virtual void set_active(bool p_active) = 0;
};

struct PropertyInfoWithDefault {
	PropertyInfo info;
	Variant def;

	PropertyInfoWithDefault(PropertyInfo &p_info, const Variant &p_def) :
			info(p_info), def(p_def) {}
};

struct Token {
	uint32_t index : 24;
	uint8_t generation = 0;

	bool is_valid() const {
		return generation > 0;
	}
};

/// This structure is used by the `SystemDispatcher` to hold some extra info
/// useful during the Dispatcher execution.
struct DispatcherSystemExecutionData {
	Token token;
	class Pipeline *pipeline;
	int dispatcher_index;
};
