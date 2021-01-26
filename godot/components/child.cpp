#include "child.h"

void Child::_bind_methods() {
	// Don't expose this to editor. It's automatically resolved at runtime.
	//ECS_BIND_PROPERTY(Child, PropertyInfo(Variant::INT, "parent"), parent);
}
