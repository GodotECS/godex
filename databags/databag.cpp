#include "databag.h"

/** @author AndreaCatania */

godex::Databag::Databag() {}

void godex::Databag::_bind_methods() {}

const LocalVector<PropertyInfo> *godex::Databag::get_properties() const {
	CRASH_NOW_MSG("The `Databag` class must always be tagged using the macro `DATABAG()`.");
	return nullptr;
}

bool godex::Databag::set(const StringName &p_name, const Variant &p_data) {
	CRASH_NOW_MSG("The `Databag` class must always be tagged using the macro `DATABAG()`.");
	return false;
}

bool godex::Databag::get(const StringName &p_name, Variant &r_data) const {
	CRASH_NOW_MSG("The `Databag` class must always be tagged using the macro `DATABAG()`.");
	return false;
}

bool godex::Databag::set(const uint32_t p_index, const Variant &p_data) {
	CRASH_NOW_MSG("The `Databag` class must always be tagged using the macro `DATABAG()`.");
	return false;
}

bool godex::Databag::get(const uint32_t p_index, Variant &r_data) const {
	CRASH_NOW_MSG("The `Databag` class must always be tagged using the macro `DATABAG()`.");
	return false;
}
