#include "ecs_resource.h"

/** @author AndreaCatania */

ECSResource::ECSResource() {
}

void ECSResource::_bind_properties() {}

OAHashMap<StringName, PropertyInfo> *ECSResource::get_properties() const {
	CRASH_NOW_MSG("The resource class must always be tagged using the macro `RESOURCE()`.");
	return nullptr;
}