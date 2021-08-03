#include "shape_cylinder.h"

void BtCylinder::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtCylinder, PropertyInfo(Variant::FLOAT, "radius"), set_radius, get_radius);
	ECS_BIND_PROPERTY_FUNC(BtCylinder, PropertyInfo(Variant::FLOAT, "height"), set_height, get_height);
}

void BtCylinder::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 50 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 50;
}

void BtCylinder::set_radius(const real_t p_radius) {
	radius = p_radius;
	update_shapes();
}

real_t BtCylinder::get_radius() const {
	return radius;
}

void BtCylinder::set_height(const real_t p_height) {
	height = p_height;
	update_shapes();
}

real_t BtCylinder::get_height() const {
	return height;
}

BtRigidShape::ShapeInfo *BtCylinder::add_shape(btCylinderShape *p_shape, const Vector3 &p_scale) {
	// Radius scaled by X
	const real_t scaled_radius = radius * p_scale.x;
	// Height scaled by Y.
	const real_t scaled_half_height = (height * p_scale.y) / 2.0;

	// No margin, since the radius is used as margin already.
	p_shape->setMargin(0.0);
	p_shape->setImplicitShapeDimensions(btVector3(scaled_radius, scaled_half_height, scaled_radius));

	return __add_shape(p_shape, p_scale);
}

void BtCylinder::update_shapes() {
	for (uint32_t i = 0; i < shapes_info.size(); i += 1) {
		btCylinderShape *shape = static_cast<btCylinderShape *>(shapes_info[i].shape_ptr);
		// Radius scaled by X
		const real_t scaled_radius = radius * shapes_info[i].scale.x;
		// Height scaled by Y.
		const real_t scaled_half_height = (height * shapes_info[i].scale.y) / 2.0;
		// No margin, since the radius is used as margin already.
		shape->setMargin(0);
		shape->setImplicitShapeDimensions(btVector3(scaled_radius, scaled_half_height, scaled_radius));
	}
}

btCollisionShape *BtShapeStorageCylinder::construct_shape(BtCylinder *p_shape_owner, const Vector3 &p_scale) {
	auto shape = allocator.alloc(btVector3(1.0, 1.0, 1.0) / 2.0);
	p_shape_owner->add_shape(shape, p_scale);
	return shape;
}
