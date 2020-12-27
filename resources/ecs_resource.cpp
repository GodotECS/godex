#include "ecs_resource.h"

/** @author AndreaCatania */

godex::Resource::Resource() {}

void godex::Resource::_bind_properties() {}

godex::resource_id godex::Resource::rid() const {
	CRASH_NOW_MSG("The `Resource` class must always be tagged using the macro `RESOURCE()`.");
	return UINT32_MAX;
}

const LocalVector<PropertyInfo> *godex::Resource::get_properties() const {
	CRASH_NOW_MSG("The `Resource` class must always be tagged using the macro `RESOURCE()`.");
	return nullptr;
}

bool godex::Resource::set(const StringName &p_name, const Variant &p_data) {
	CRASH_NOW_MSG("The `Resource` class must always be tagged using the macro `RESOURCE()`.");
	return false;
}

bool godex::Resource::get(const StringName &p_name, Variant &r_data) const {
	CRASH_NOW_MSG("The `Resource` class must always be tagged using the macro `RESOURCE()`.");
	return false;
}

bool godex::Resource::set(const uint32_t p_index, const Variant &p_data) {
	CRASH_NOW_MSG("The `Resource` class must always be tagged using the macro `RESOURCE()`.");
	return false;
}

bool godex::Resource::get(const uint32_t p_index, Variant &r_data) const {
	CRASH_NOW_MSG("The `Resource` class must always be tagged using the macro `RESOURCE()`.");
	return false;
}

Variant godex::Resource::get(const StringName &p_name) const {
	Variant r;
	get(p_name, r);
	return r;
}

AccessResource::AccessResource() {}

bool AccessResource::_setv(const StringName &p_name, const Variant &p_data) {
	ERR_FAIL_COND_V_MSG(__resource == nullptr, false, "This Resource is not found.");
	ERR_FAIL_COND_V_MSG(__mut == false, false, "This `Resource` was taken as not mutable.");
	return __resource->set(p_name, p_data);
}

bool AccessResource::_getv(const StringName &p_name, Variant &r_data) const {
	ERR_FAIL_COND_V_MSG(__resource == nullptr, false, "This Resource is not found.");
	return __resource->get(p_name, r_data);
}

bool AccessResource::is_mutable() const {
	return __mut;
}
