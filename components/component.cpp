#include "component.h"

/** @author AndreaCatania */

using godex::Component;

Component::Component() {}

void Component::_bind_methods() {}

godex::component_id Component::cid() const {
	CRASH_NOW_MSG("The component class must always be tagged using the macro `COMPONENT()`.");
	return UINT32_MAX;
}

const LocalVector<PropertyInfo> *Component::get_properties() const {
	CRASH_NOW_MSG("The component class must always be tagged using the macro `COMPONENT()`.");
	return nullptr;
}

bool Component::set(const StringName &p_name, const Variant &p_data) {
	CRASH_NOW_MSG("The component class must always be tagged using the macro `COMPONENT()`.");
	return false;
}

bool Component::get(const StringName &p_name, Variant &r_data) const {
	CRASH_NOW_MSG("The component class must always be tagged using the macro `COMPONENT()`.");
	return false;
}

bool Component::set(const uint32_t p_index, const Variant &p_data) {
	CRASH_NOW_MSG("The component class must always be tagged using the macro `COMPONENT()`.");
	return false;
}

bool Component::get(const uint32_t p_index, Variant &r_data) const {
	CRASH_NOW_MSG("The component class must always be tagged using the macro `COMPONENT()`.");
	return false;
}

Variant Component::get(const StringName &p_name) const {
	Variant r;
	get(p_name, r);
	return r;
}

void Component::call(const StringName &p_method, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error) {
	CRASH_NOW_MSG("The component class must always be tagged using the macro `COMPONENT()`.");
}
