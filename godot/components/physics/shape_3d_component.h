#pragma once

#include "../../../../godex/components/component.h"
#include "scene/resources/shape_3d.h"

struct Shape3DComponent : public godex::Component {
	COMPONENT(Shape3DComponent, DenseVectorStorage)
	static void _bind_methods();

public:
	Ref<Shape3D> shape;
};
