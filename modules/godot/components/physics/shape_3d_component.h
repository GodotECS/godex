#pragma once

#include "../../../../components/component.h"
#include "../../../../storage/dense_vector_storage.h"
#include "scene/resources/shape_3d.h"

struct Shape3DComponent {
	COMPONENT(Shape3DComponent, DenseVectorStorage)
	static void _bind_methods();

	Ref<Shape3D> shape;
};
