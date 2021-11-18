#include "child.h"

void Child::_bind_methods() {
	ECS_BIND_PROPERTY(Child, PropertyInfo(Variant::INT, "parent", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), parent);
}

void Child::_get_storage_config(Dictionary &r_dictionary) {
	r_dictionary["pre_allocate"] = 1000;
}

Child::Child(EntityID p_parent) :
		parent(p_parent) {
}
