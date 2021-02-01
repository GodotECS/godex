#include "child.h"

void Child::_bind_methods() {
	// TODO don't expose to editor but allow fetch this from scripts.
	ECS_BIND_PROPERTY(Child, PropertyInfo(Variant::INT, "parent"), parent);
}

Child::Child() {
}

Child::Child(EntityID p_parent) :
		parent(p_parent) {
}
