#include "components_rigid_shape.h"

btCollisionShape *BtRigidShape::get_shape() {
	switch (type) {
		case TYPE_BOX:
			return &static_cast<BtShapeBox *>(this)->box;
		case TYPE_SPHERE:
			return &static_cast<BtShapeSphere *>(this)->sphere;
		case TYPE_CAPSULE:
			return &static_cast<BtShapeCapsule *>(this)->capsule;
		case TYPE_CONE:
			return &static_cast<BtShapeCone *>(this)->cone;
		case TYPE_CYLINDER:
			return &static_cast<BtShapeCylinder *>(this)->cylinder;
		case TYPE_WORLD_MARGIN:
			return &static_cast<BtShapeWorldMargin *>(this)->world_margin;
		case TYPE_CONVEX:
			return &static_cast<BtShapeConvex *>(this)->convex;
		case TYPE_TRIMESH:
			return static_cast<BtShapeTrimesh *>(this)->trimesh;
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
	r_config["page_size"] = 200;
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

void BtShapeSphere::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtShapeSphere, PropertyInfo(Variant::FLOAT, "radius"), set_radius, get_radius);
}

void BtShapeSphere::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 500 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 50;
}

void BtShapeSphere::set_radius(real_t p_radius) {
	sphere.setUnscaledRadius(p_radius);
}

real_t BtShapeSphere::get_radius() const {
	return sphere.getRadius();
}

void BtShapeCapsule::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtShapeCapsule, PropertyInfo(Variant::FLOAT, "radius"), set_radius, get_radius);
	ECS_BIND_PROPERTY_FUNC(BtShapeCapsule, PropertyInfo(Variant::FLOAT, "height"), set_height, get_height);
}

void BtShapeCapsule::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 500 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 50;
}

void BtShapeCapsule::set_radius(real_t p_radius) {
	// The capsule margin is the radius of the capsule.
	capsule.setMargin(p_radius);
	capsule.setImplicitShapeDimensions(btVector3(p_radius, capsule.getHalfHeight(), p_radius));
}

real_t BtShapeCapsule::get_radius() const {
	return capsule.getRadius();
}

void BtShapeCapsule::set_height(real_t p_height) {
	capsule.setImplicitShapeDimensions(btVector3(capsule.getRadius(), 0.5f * p_height, capsule.getRadius()));
}

real_t BtShapeCapsule::get_height() const {
	return capsule.getHalfHeight() * 2.0;
}

void BtShapeCone::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtShapeCone, PropertyInfo(Variant::FLOAT, "radius"), set_radius, get_radius);
	ECS_BIND_PROPERTY_FUNC(BtShapeCone, PropertyInfo(Variant::FLOAT, "height"), set_height, get_height);
	ECS_BIND_PROPERTY_FUNC(BtShapeCone, PropertyInfo(Variant::FLOAT, "margin"), set_margin, get_margin);
}

void BtShapeCone::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 500 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 50;
}

void BtShapeCone::set_radius(real_t p_radius) {
	cone.setRadius(p_radius);
}

real_t BtShapeCone::get_radius() const {
	return cone.getRadius();
}

void BtShapeCone::set_height(real_t p_height) {
	cone.setHeight(p_height);
}

real_t BtShapeCone::get_height() const {
	return cone.getHeight();
}

void BtShapeCone::set_margin(real_t p_margin) {
	cone.setMargin(p_margin);
}

real_t BtShapeCone::get_margin() const {
	return cone.getMargin();
}

void BtShapeCylinder::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtShapeCylinder, PropertyInfo(Variant::FLOAT, "radius"), set_radius, get_radius);
	ECS_BIND_PROPERTY_FUNC(BtShapeCylinder, PropertyInfo(Variant::FLOAT, "height"), set_height, get_height);
	ECS_BIND_PROPERTY_FUNC(BtShapeCylinder, PropertyInfo(Variant::FLOAT, "margin"), set_margin, get_margin);
}

void BtShapeCylinder::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 500 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 50;
}

void BtShapeCylinder::set_radius(real_t p_radius) {
	cylinder.setImplicitShapeDimensions(btVector3(p_radius - get_margin(), (get_height() * 0.5) - get_margin(), p_radius - get_margin()));
}

real_t BtShapeCylinder::get_radius() const {
	return cylinder.getRadius();
}

void BtShapeCylinder::set_height(real_t p_height) {
	cylinder.setImplicitShapeDimensions(btVector3(get_radius() - get_margin(), (p_height * 0.5) - get_margin(), get_radius() - get_margin()));
}

real_t BtShapeCylinder::get_height() const {
	return cylinder.getHalfExtentsWithMargin()[1] * 2.0;
}

void BtShapeCylinder::set_margin(real_t p_margin) {
	// This already handles the Radius and Height adjustment.
	cylinder.setMargin(p_margin);
}

real_t BtShapeCylinder::get_margin() const {
	return cylinder.getMargin();
}

void BtShapeWorldMargin::_bind_methods() {
}

void BtShapeWorldMargin::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 500 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 4;
}

void BtShapeConvex::_bind_methods() {
}

void BtShapeConvex::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 500 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 200;
}

void BtShapeTrimesh::_bind_methods() {
}

void BtShapeTrimesh::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 500 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 200;
}
