#include "shape_sphere.h"

void BtSphere::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtSphere, PropertyInfo(Variant::FLOAT, "radius"), set_radius, get_radius);
}

void BtSphere::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 500 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 200;
}

void BtSphere::set_radius(real_t p_radius) {
	for (uint32_t i = 0; i < shapes_info.size(); i += 1) {
		static_cast<btSphereShape *>(shapes_info[i].shape_ptr)->setUnscaledRadius(p_radius * shapes_info[i].scale[0]);
	}
	radius = p_radius;
}

real_t BtSphere::get_radius() const {
	return radius;
}

BtRigidShape::ShapeInfo *BtSphere::add_shape(btSphereShape *p_shape, const Vector3 &p_scale) {
	p_shape->setUnscaledRadius(radius * p_scale[0]);
	return __add_shape(p_shape, p_scale);
}

btCollisionShape *BtShapeStorageSphere::construct_shape(BtSphere *p_shape_owner, const Vector3 &p_scale) {
	auto shape = allocator.alloc(1.0);
	p_shape_owner->add_shape(shape, p_scale);
	return shape;
}
