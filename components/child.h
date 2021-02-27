#pragma once

#include "component.h"

class Child {
	COMPONENT_CUSTOM_STORAGE(Child)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_dictionary);

public:
	// The parent `Entity`.
	EntityID parent;
	// First child of next level.
	EntityID first_child;
	// The next child in this level
	EntityID next;

	Child(EntityID p_parent);
};
