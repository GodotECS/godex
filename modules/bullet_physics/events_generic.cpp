#include "events_generic.h"

void OverlapStart::_bind_method() {
	ECS_BIND_PROPERTY(OverlapStart, PropertyInfo(Variant::INT, "area"), area);
	ECS_BIND_PROPERTY(OverlapStart, PropertyInfo(Variant::INT, "other_body"), other_body);
}

void OverlapEnd::_bind_method() {
	ECS_BIND_PROPERTY(OverlapEnd, PropertyInfo(Variant::INT, "area"), area);
	ECS_BIND_PROPERTY(OverlapEnd, PropertyInfo(Variant::INT, "other_body"), other_body);
}
