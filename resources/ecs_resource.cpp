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
