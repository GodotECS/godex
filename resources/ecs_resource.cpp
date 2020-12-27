#include "ecs_resource.h"

/** @author AndreaCatania */

godex::Resource::Resource() {}

void godex::Resource::_bind_properties() {}

OAHashMap<StringName, PropertyInfo> *godex::Resource::get_properties() const {
	CRASH_NOW_MSG("The resource class must always be tagged using the macro `RESOURCE()`.");
	return nullptr;
}

AccessResource::AccessResource() {}

bool AccessResource::_setv(const StringName &p_name, const Variant &p_data) {
	// TODO
	return false;
}

bool AccessResource::_getv(const StringName &p_name, Variant &r_data) const {
	// TODO
	return false;
}

bool AccessResource::is_mutable() const {
	return mut;
}
