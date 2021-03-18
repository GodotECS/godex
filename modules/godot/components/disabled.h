#pragma once

#include "../../../components/component.h"
#include "../../../storage/dense_vector_storage.h"

// Tag component to mark `Disabled` things.
struct Disabled {
	COMPONENT(Disabled, DenseVectorStorage)
	static void _bind_methods() {}
};
