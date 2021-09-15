#pragma once

#include "../../../components/component.h"
#include "../../../storage/dense_vector_storage.h"
#include "core/math/transform_3d.h"

class InterpolatedTransformComponent {
	COMPONENT(InterpolatedTransformComponent, DenseVectorStorage)

	Vector3 previous_linear_velocity;
	Vector3 current_linear_velocity;

	Transform3D previous_transform;
	Transform3D current_transform;
};
