#include "shape_cone.h"

void BtCone::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtCone, PropertyInfo(Variant::FLOAT, "radius"), set_radius, get_radius);
	ECS_BIND_PROPERTY_FUNC(BtCone, PropertyInfo(Variant::FLOAT, "height"), set_height, get_height);
	ECS_BIND_PROPERTY_FUNC(BtCone, PropertyInfo(Variant::FLOAT, "margin"), set_margin, get_margin);
}

void BtCone::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 50 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 50;
}

void BtCone::set_radius(const real_t p_radius) {
	radius = p_radius;
	update_shapes();
}

real_t BtCone::get_radius() const {
	return radius;
}

void BtCone::set_height(const real_t p_height) {
	height = p_height;
	update_shapes();
}

real_t BtCone::get_height() const {
	return height;
}

void BtCone::set_margin(real_t p_margin) {
	margin = p_margin;
	update_shapes();
}

real_t BtCone::get_margin() const {
	return margin;
}

BtRigidShape::ShapeInfo *BtCone::add_shape(btConeShape *p_shape, const Vector3 &p_scale) {
	// Radius scaled by X
	const real_t scaled_radius = radius * p_scale.x;
	// Height scaled by Y.
	const real_t scaled_height = (height * p_scale.y);

	p_shape->setRadius(scaled_radius);
	p_shape->setHeight(scaled_height);
	p_shape->setMargin(margin);

	return __add_shape(p_shape, p_scale);
}

void BtCone::update_shapes() {
	for (uint32_t i = 0; i < shapes_info.size(); i += 1) {
		btConeShape *shape = static_cast<btConeShape *>(shapes_info[i].shape_ptr);
		// Radius scaled by X
		const real_t scaled_radius = radius * shapes_info[i].scale.x;
		// Height scaled by Y.
		const real_t scaled_height = height * shapes_info[i].scale.y;
		shape->setRadius(scaled_radius);
		shape->setHeight(scaled_height);
		shape->setMargin(margin);
	}
}

btCollisionShape *BtShapeStorageCone::construct_shape(BtCone *p_shape_owner, const Vector3 &p_scale) {
	auto shape = allocator.alloc(/*Base Radius*/ 1.0, /*Height*/ 1.0);
	p_shape_owner->add_shape(shape, p_scale);
	return shape;
}
