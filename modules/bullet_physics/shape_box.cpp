#include "shape_box.h"

void BtBox::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtBox, PropertyInfo(Variant::VECTOR3, "half_extents"), set_half_extents, get_half_extents);
	ECS_BIND_PROPERTY_FUNC(BtBox, PropertyInfo(Variant::FLOAT, "margin"), set_margin, get_margin);
}

void BtBox::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 500 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 200;
}

void BtBox::set_half_extents(const Vector3 &p_half_extends) {
	for (uint32_t i = 0; i < shapes_info.size(); i += 1) {
		btBoxShape *shape = static_cast<btBoxShape *>(shapes_info[i].shape_ptr);
		const Vector3 extends = p_half_extends * shapes_info[i].scale;
		shape->setImplicitShapeDimensions(
				btVector3(
						extends.x - shape->getMargin(),
						extends.y - shape->getMargin(),
						extends.z - shape->getMargin()));
	}
	half_extends = p_half_extends;
}

Vector3 BtBox::get_half_extents() const {
	return half_extends;
}

void BtBox::set_margin(real_t p_margin) {
	for (uint32_t i = 0; i < shapes_info.size(); i += 1) {
		static_cast<btBoxShape *>(shapes_info[i].shape_ptr)->setMargin(p_margin * shapes_info[i].scale.length());
	}
	margin = p_margin;
}

real_t BtBox::get_margin() const {
	return margin;
}

BtRigidShape::ShapeInfo *BtBox::add_shape(btBoxShape *p_shape, const Vector3 &p_scale) {
	p_shape->setMargin(margin * p_scale.length());
	const Vector3 extends = half_extends * p_scale;
	p_shape->setImplicitShapeDimensions(
			btVector3(
					extends.x - p_shape->getMargin(),
					extends.y - p_shape->getMargin(),
					extends.z - p_shape->getMargin()));
	return __add_shape(p_shape, p_scale);
}

btCollisionShape *BtShapeStorageBox::construct_shape(BtBox *p_shape_owner, const Vector3 &p_scale) {
	auto shape = allocator.alloc(btVector3(1.0, 1.0, 1.0));
	p_shape_owner->add_shape(shape, p_scale);
	return shape;
}
