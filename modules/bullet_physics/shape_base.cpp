#include "shape_base.h"

#include "bullet_types_converter.h"

bool BtRigidShape::fallback_empty() const {
	return type == TYPE_SHAPE_CONTAINER;
}

BtRigidShape::ShapeInfo *BtRigidShape::get_shape(const Vector3 &p_margin) {
	return const_cast<BtRigidShape::ShapeInfo *>(const_cast<const BtRigidShape *>(this)->get_shape(p_margin));
}

const BtRigidShape::ShapeInfo *BtRigidShape::get_shape(const Vector3 &p_scale) const {
	for (uint32_t i = 0; i < shapes_info.size(); i += 1) {
		if ((shapes_info[i].scale - p_scale).length_squared() <= CMP_EPSILON2) {
			// Fantastic, We have a shape with this scaling already!
			return shapes_info.ptr() + i;
		}
	}

	// No shape found with this scaling.
	return nullptr;
}

BtRigidShape::ShapeInfo *BtRigidShape::__add_shape(btCollisionShape *p_shape, const Vector3 &p_scale) {
	const uint32_t index = shapes_info.size();
	shapes_info.push_back(ShapeInfo());
	shapes_info[index].shape_ptr = p_shape;
	shapes_info[index].scale = p_scale;
	return shapes_info.ptr() + index;
}
