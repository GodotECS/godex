#pragma once

/* Author: AndreaCatania */

#include "core/object/object.h"
#include "core/object/script_language.h"
#include "core/string/ustring.h"
#include "core/templates/list.h"
#include "core/templates/local_vector.h"
#include "modules/gdscript/gdscript.h"

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
	uint32_t get_property_index(const StringName &p_name) const {                                                                      \
		const int64_t i = property_map.find(p_name);                                                                                   \
		return i == -1 ? UINT32_MAX : uint32_t(i);                                                                                     \
	}                                                                                                                                  \
																																	   \
public:                                                                                                                                \
	virtual bool set(const StringName &p_name, const Variant &p_data) override {                                                       \
		const uint32_t i = get_property_index(p_name);                                                                                 \
		ERR_FAIL_COND_V_MSG(i == UINT32_MAX, false, "The parameter " + p_name + " doesn't exist in this component.");                  \
		return setters[i](this, p_data);                                                                                               \
	}                                                                                                                                  \
	virtual bool get(const StringName &p_name, Variant &r_data) const override {                                                       \
		const uint32_t i = get_property_index(p_name);                                                                                 \
		ERR_FAIL_COND_V_MSG(i == UINT32_MAX, false, "The parameter " + p_name + " doesn't exist in this component.");                  \
		return getters[i](this, r_data);                                                                                               \
	}                                                                                                                                  \
	virtual bool set(const uint32_t p_index, const Variant &p_data) override {                                                         \
		ERR_FAIL_COND_V_MSG(p_index >= setters.size(), false, "The parameter " + itos(p_index) + " doesn't exist in this component."); \
		return setters[p_index](this, p_data);                                                                                         \
	}                                                                                                                                  \
	virtual bool get(const uint32_t p_index, Variant &r_data) const override {                                                         \
		ERR_FAIL_COND_V_MSG(p_index >= getters.size(), false, "The parameter " + itos(p_index) + " doesn't exist in this component."); \
		return getters[p_index](this, r_data);                                                                                         \
	}

/// Must be called in `_bind_property` and can be used to just bind a property.
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

/// Must be called in `_bind_property` and can be used to bind set and get of a
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

/// This is useful to access the storages fast. Since `Object::set` check fist
/// the script. However, in future would be really nice make `Object::set` virtual
/// so to override it and avoid all this useless extra work.
template <class E>
class DataAccessorScriptInstance : public ScriptInstance {
public:
	Object *owner = nullptr;
	E *__target = nullptr;
	bool __mut = false;

	bool is_mutable() const {
		return __mut;
	}

	virtual bool set(const StringName &p_name, const Variant &p_value) override {
		ERR_FAIL_COND_V(__target == nullptr, false);
		ERR_FAIL_COND_V_MSG(__mut == false, false, "This element was taken as not mutable.");
		return __target->set(p_name, p_value);
	}

	// Slow version
	Variant get(const StringName &p_name) const {
		Variant ret;
		get(p_name, ret);
		return ret;
	}

	virtual bool get(const StringName &p_name, Variant &r_ret) const override {
		ERR_FAIL_COND_V(__target == nullptr, false);
		return __target->get(p_name, r_ret);
	}

	virtual void get_property_list(List<PropertyInfo> *p_properties) const override {
		ERR_FAIL_COND(__target == nullptr);
		const LocalVector<PropertyInfo> *props = __target->get_properties();
		for (uint32_t i = 0; i < props->size(); i += 1) {
			p_properties->push_back((*props)[i]);
		}
	}

	virtual Variant::Type get_property_type(const StringName &p_name, bool *r_is_valid = nullptr) const override {
		ERR_FAIL_COND_V(__target == nullptr, Variant::NIL);
		const LocalVector<PropertyInfo> *props = __target->get_properties();
		for (uint32_t i = 0; i < props->size(); i += 1) {
			if ((*props)[i].name == String(p_name)) {
				return (*props)[i].type;
				(*r_is_valid) = true;
			}
		}
		(*r_is_valid) = false;
		return Variant::NIL;
	}

	virtual Object *get_owner() override { return owner; }
	virtual void get_property_state(List<Pair<StringName, Variant>> &state) override {
		// This is used by the scene packer to store the script data.
		// TODO Is this needed, implement this?
	}

	virtual void get_method_list(List<MethodInfo> *p_list) const {
		p_list->push_back(MethodInfo(Variant::BOOL, "is_valid"));
		p_list->push_back(MethodInfo(Variant::BOOL, "is_mutable"));
	}

	virtual bool has_method(const StringName &p_method) const {
		return String(p_method) == "is_valid";
	}

	virtual Variant call(const StringName &p_method, VARIANT_ARG_LIST) override {
		if (String(p_method) == "is_valid") {
			return __target != nullptr;
		} else if (String(p_method) == "is_mutable") {
			return __mut;
		}
		return Variant();
	}

	virtual Variant call(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) override {
		if (String(p_method) == "is_valid") {
			r_error.error = Callable::CallError::Error::CALL_OK;
			return __target != nullptr;
		} else if (String(p_method) == "is_mutable") {
			r_error.error = Callable::CallError::Error::CALL_OK;
			return __mut;
		} else {
			r_error.error = Callable::CallError::Error::CALL_ERROR_INVALID_METHOD;
			return Variant();
		}
	}

	virtual void notification(int p_notification) override {
		// Nothing to do.
	}

	virtual Ref<Script> get_script() const override {
		// Just to return something to make godot not crash.
		Ref<GDScript> s;
		s.instance();
		return s;
	}

	virtual bool is_placeholder() const override {
		return false;
	}

	virtual Vector<ScriptNetData> get_rpc_methods() const override {
		return Vector<ScriptNetData>();
	}
	virtual uint16_t get_rpc_method_id(const StringName &p_method) const override {
		return 0;
	}
	virtual StringName get_rpc_method(uint16_t p_id) const override {
		return "";
	}
	virtual MultiplayerAPI::RPCMode get_rpc_mode_by_id(uint16_t p_id) const override {
		return MultiplayerAPI::RPC_MODE_DISABLED;
	}
	virtual MultiplayerAPI::RPCMode get_rpc_mode(const StringName &p_method) const override {
		return MultiplayerAPI::RPC_MODE_DISABLED;
	}

	virtual Vector<ScriptNetData> get_rset_properties() const override {
		return Vector<ScriptNetData>();
	}
	virtual uint16_t get_rset_property_id(const StringName &p_variable) const override { return 0; }
	virtual StringName get_rset_property(uint16_t p_id) const override { return ""; }
	virtual MultiplayerAPI::RPCMode get_rset_mode_by_id(uint16_t p_id) const override { return MultiplayerAPI::RPC_MODE_DISABLED; }
	virtual MultiplayerAPI::RPCMode get_rset_mode(const StringName &p_variable) const override { return MultiplayerAPI::RPC_MODE_DISABLED; }

	virtual ScriptLanguage *get_language() override { return nullptr; }

	virtual ~DataAccessorScriptInstance() {
	}
};
