#include "component.h"

/** @author AndreaCatania */

using godex::Component;

Component::Component() {}

void Component::_bind_methods() {}

const LocalVector<PropertyInfo> *Component::get_properties() const {
	CRASH_NOW_MSG("The component class must always be tagged using the macro `COMPONENT()`.");
	return nullptr;
}
