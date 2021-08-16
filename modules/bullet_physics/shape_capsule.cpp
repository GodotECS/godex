#include "shape_capsule.h"

void GodexBtCapsuleShape::set_radius(real_t p_radius) {
	m_collisionMargin = p_radius;
	m_implicitShapeDimensions.setX(p_radius);
	m_implicitShapeDimensions.setZ(p_radius);
}

void GodexBtCapsuleShape::set_height(real_t p_height) {
	m_implicitShapeDimensions.setY(p_height / 2.0);
}

void BtCapsule::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtCapsule, PropertyInfo(Variant::FLOAT, "radius"), set_radius, get_radius);
	ECS_BIND_PROPERTY_FUNC(BtCapsule, PropertyInfo(Variant::FLOAT, "height"), set_height, get_height);
}

void BtCapsule::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 50 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 50;
}

void BtCapsule::set_radius(const real_t p_radius) {
	radius = p_radius;
	update_shapes();
}

real_t BtCapsule::get_radius() const {
	return radius;
}

void BtCapsule::set_height(const real_t p_height) {
	height = p_height;
	update_shapes();
}

real_t BtCapsule::get_height() const {
	return height;
}

BtRigidShape::ShapeInfo *BtCapsule::add_shape(GodexBtCapsuleShape *p_shape, const Vector3 &p_scale) {
	// Radius scaled by X
	const real_t scaled_radius = radius * p_scale.x;
	// Height scaled by Y.
	const real_t scaled_height = (height * p_scale.y);
	p_shape->set_radius(scaled_radius);
	p_shape->set_height(scaled_height);
	return __add_shape(p_shape, p_scale);
}

void BtCapsule::update_shapes() {
	for (uint32_t i = 0; i < shapes_info.size(); i += 1) {
		GodexBtCapsuleShape *shape = static_cast<GodexBtCapsuleShape *>(shapes_info[i].shape_ptr);
		// Radius scaled by X
		const real_t scaled_radius = radius * shapes_info[i].scale.x;
		// Height scaled by Y.
		const real_t scaled_height = (height * shapes_info[i].scale.y);
		shape->set_radius(scaled_radius);
		shape->set_height(scaled_height);
	}
}

btCollisionShape *BtShapeStorageCapsule::construct_shape(BtCapsule *p_capsule, const Vector3 &p_scale) {
	auto shape = allocator.alloc();
	p_capsule->add_shape(shape, p_scale);
	return shape;
}
