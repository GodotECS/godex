#include "databag.h"

/** @author AndreaCatania */

godex::Databag::Databag() {}

void godex::Databag::_bind_methods() {}

godex::databag_id godex::Databag::rid() const {
	CRASH_NOW_MSG("The `Databag` class must always be tagged using the macro `DATABAG()`.");
	return UINT32_MAX;
}

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

Variant godex::Databag::get(const StringName &p_name) const {
	Variant r;
	get(p_name, r);
	return r;
}

void godex::Databag::call(const StringName &p_method, const Variant **p_args, int p_argcount, Variant *r_ret, Callable::CallError &r_error) {
	CRASH_NOW_MSG("The `Databag` class must always be tagged using the macro `DATABAG()`.");
}
