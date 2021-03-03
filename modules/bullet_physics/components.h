#pragma once

#include "../../components/component.h"

/// This is an optional component, that allow to specify a specific physics
/// `Space` where the `Entity` is put.
/// When this component is not set, the `Entity` is put to the default main
/// space.
///
/// Note, you have at max 4 spaces, and it's unlikely that you need more than 1.
struct BtSpaceMarker {
	COMPONENT(BtSpaceMarker, DenseVectorStorage)

	static void _bind_methods() {
		ECS_BIND_PROPERTY(BtSpaceMarker, PropertyInfo(Variant::INT, "space_id", PROPERTY_HINT_ENUM, "Space 0 (main),Space 1,Space 2,Space 3"), space_id);
	}

	uint32_t space_id = 0;
};
