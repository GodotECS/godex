#include "component.h"

/** @author AndreaCatania */

using godex::AccessComponent;
using godex::Component;

Component::Component() {}

void Component::_bind_properties() {}

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

AccessComponent::AccessComponent() {}

bool AccessComponent::_setv(const StringName &p_name, const Variant &p_data) {
	ERR_FAIL_COND_V(__component == nullptr, false);
	ERR_FAIL_COND_V_MSG(__mut == false, false, "This component was taken as not mutable.");
	return __component->set(p_name, p_data);
}

bool AccessComponent::_getv(const StringName &p_name, Variant &r_data) const {
	ERR_FAIL_COND_V(__component == nullptr, false);
	return __component->get(p_name, r_data);
}

bool AccessComponent::is_mutable() const {
	return __mut;
}
