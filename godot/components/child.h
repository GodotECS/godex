#pragma once

#include "../../components/component.h"

class Child : public godex::Component {
	COMPONENT(Child, DenseVectorStorage)

	static void _bind_methods();

public:
	EntityID parent;

	Child();
	Child(EntityID p_parent);
};
