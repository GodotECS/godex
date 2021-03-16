#include "components_rigid_shape.h"

btCollisionShape *BtRigidShape::get_shape() {
	switch (type) {
		case TYPE_BOX:
			return &static_cast<BtShapeBox *>(this)->box;
		case TYPE_SPHERE:
			return &static_cast<BtShapeSphere *>(this)->sphere;
	}

	CRASH_NOW_MSG("Please support all shapes.");
	return nullptr;
}

const btCollisionShape *BtRigidShape::get_shape() const {
	return const_cast<BtRigidShape *>(this)->get_shape();
}

void BtShapeBox::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtShapeBox, PropertyInfo(Variant::VECTOR3, "half_extents"), set_half_extents, get_half_extents);
	ECS_BIND_PROPERTY_FUNC(BtShapeBox, PropertyInfo(Variant::FLOAT, "margin"), set_margin, get_margin);
}

void BtShapeBox::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 500 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 500;
}

void BtShapeBox::set_half_extents(const Vector3 &p_half_extends) {
	box.setImplicitShapeDimensions(btVector3(p_half_extends.x - box.getMargin(), p_half_extends.y - box.getMargin(), p_half_extends.z - box.getMargin()));
}

Vector3 BtShapeBox::get_half_extents() const {
	const btVector3 &extents = box.getImplicitShapeDimensions();
	return Vector3(extents.x() + box.getMargin(), extents.y() + box.getMargin(), extents.z() + box.getMargin());
}

void BtShapeBox::set_margin(real_t p_margin) {
	box.setMargin(p_margin);
}

real_t BtShapeBox::get_margin() const {
	return box.getMargin();
}
