#include "bt_systems.h"

void bt_config_shape(
		Query<BtRigidBody, Flatten<Changed<BtShapeBox>, Changed<BtShapeSphere>>> &p_query) {
	for (auto [body, shape_container] : p_query) {
		BtRigidShape *shape = shape_container.as<BtRigidShape>();
		CRASH_COND(shape != nullptr);
	}
}

void bt_config_body(
		BtWorld *p_space,
		Query<Changed<BtRigidBody>, Maybe<BtWorldMarker>> &p_query) {
}
