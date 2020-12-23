#include "ecs_resource.h"

/** @author AndreaCatania */

godex::Resource::Resource() {}

void godex::Resource::_bind_properties() {}

OAHashMap<StringName, PropertyInfo> *godex::Resource::get_properties() const {
	CRASH_NOW_MSG("The resource class must always be tagged using the macro `RESOURCE()`.");
	return nullptr;
}
