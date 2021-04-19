#include "components_rigid_shape.h"

#include "bullet_types_converter.h"
#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>
#include <stdio.h>

btCollisionShape *BtRigidShape::get_shape() {
	switch (type) {
		case TYPE_BOX:
			return &static_cast<BtBox *>(this)->box;
		case TYPE_SPHERE:
			return &static_cast<BtSphere *>(this)->sphere;
		case TYPE_CAPSULE:
			return &static_cast<BtCapsule *>(this)->capsule;
		case TYPE_CONE:
			return &static_cast<BtCone *>(this)->cone;
		case TYPE_CYLINDER:
			return &static_cast<BtCylinder *>(this)->cylinder;
		case TYPE_WORLD_MARGIN:
			return &static_cast<BtWorldMargin *>(this)->world_margin;
		case TYPE_CONVEX:
			return &static_cast<BtConvex *>(this)->convex;
		case TYPE_TRIMESH:
			return static_cast<BtTrimesh *>(this)->trimesh;
	}

	CRASH_NOW_MSG("Please support all shapes.");
	return nullptr;
}

const btCollisionShape *BtRigidShape::get_shape() const {
	return const_cast<BtRigidShape *>(this)->get_shape();
}

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
	box.setImplicitShapeDimensions(btVector3(p_half_extends.x - box.getMargin(), p_half_extends.y - box.getMargin(), p_half_extends.z - box.getMargin()));
}

Vector3 BtBox::get_half_extents() const {
	const btVector3 &extents = box.getImplicitShapeDimensions();
	return Vector3(extents.x() + box.getMargin(), extents.y() + box.getMargin(), extents.z() + box.getMargin());
}

void BtBox::set_margin(real_t p_margin) {
	box.setMargin(p_margin);
}

real_t BtBox::get_margin() const {
	return box.getMargin();
}

void BtSphere::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtSphere, PropertyInfo(Variant::FLOAT, "radius"), set_radius, get_radius);
}

void BtSphere::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 500 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 50;
}

void BtSphere::set_radius(real_t p_radius) {
	sphere.setUnscaledRadius(p_radius);
}

real_t BtSphere::get_radius() const {
	return sphere.getRadius();
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

void BtCapsule::set_radius(real_t p_radius) {
	// The capsule margin is the radius of the capsule.
	capsule.setMargin(p_radius);
	capsule.setImplicitShapeDimensions(btVector3(p_radius, capsule.getHalfHeight(), p_radius));
}

real_t BtCapsule::get_radius() const {
	return capsule.getRadius();
}

void BtCapsule::set_height(real_t p_height) {
	capsule.setImplicitShapeDimensions(btVector3(capsule.getRadius(), 0.5f * p_height, capsule.getRadius()));
}

real_t BtCapsule::get_height() const {
	return capsule.getHalfHeight() * 2.0;
}

void BtCone::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtCone, PropertyInfo(Variant::FLOAT, "radius"), set_radius, get_radius);
	ECS_BIND_PROPERTY_FUNC(BtCone, PropertyInfo(Variant::FLOAT, "height"), set_height, get_height);
	ECS_BIND_PROPERTY_FUNC(BtCone, PropertyInfo(Variant::FLOAT, "margin"), set_margin, get_margin);
}

void BtCone::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 500 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 50;
}

void BtCone::set_radius(real_t p_radius) {
	cone.setRadius(p_radius);
}

real_t BtCone::get_radius() const {
	return cone.getRadius();
}

void BtCone::set_height(real_t p_height) {
	cone.setHeight(p_height);
}

real_t BtCone::get_height() const {
	return cone.getHeight();
}

void BtCone::set_margin(real_t p_margin) {
	cone.setMargin(p_margin);
}

real_t BtCone::get_margin() const {
	return cone.getMargin();
}

void BtCylinder::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtCylinder, PropertyInfo(Variant::FLOAT, "radius"), set_radius, get_radius);
	ECS_BIND_PROPERTY_FUNC(BtCylinder, PropertyInfo(Variant::FLOAT, "height"), set_height, get_height);
	ECS_BIND_PROPERTY_FUNC(BtCylinder, PropertyInfo(Variant::FLOAT, "margin"), set_margin, get_margin);
}

void BtCylinder::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 500 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 50;
}

void BtCylinder::set_radius(real_t p_radius) {
	cylinder.setImplicitShapeDimensions(btVector3(p_radius - get_margin(), (get_height() * 0.5) - get_margin(), p_radius - get_margin()));
}

real_t BtCylinder::get_radius() const {
	return cylinder.getRadius();
}

void BtCylinder::set_height(real_t p_height) {
	cylinder.setImplicitShapeDimensions(btVector3(get_radius() - get_margin(), (p_height * 0.5) - get_margin(), get_radius() - get_margin()));
}

real_t BtCylinder::get_height() const {
	return cylinder.getHalfExtentsWithMargin()[1] * 2.0;
}

void BtCylinder::set_margin(real_t p_margin) {
	// This already handles the Radius and Height adjustment.
	cylinder.setMargin(p_margin);
}

real_t BtCylinder::get_margin() const {
	return cylinder.getMargin();
}

void BtWorldMargin::_bind_methods() {
}

void BtWorldMargin::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 500 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 4;
}

BtConvex::BtConvex(const BtConvex &p_other) :
		BtRigidShape(TYPE_CONVEX) {
	operator=(p_other);
}

BtConvex &BtConvex::operator=(const BtConvex &p_other) {
	point_count = p_other.point_count;

	if (point_count > 0) {
		points = (btVector3 *)memrealloc(points, point_count * sizeof(btVector3));
		memcpy(points, p_other.points, point_count * sizeof(btVector3));
	} else if (points != nullptr) {
		memdelete(points);
		points = nullptr;
	}

	update_internal_shape();
	return *this;
}

BtConvex::~BtConvex() {
	if (points != nullptr) {
		memdelete(points);
	}
}

void BtConvex::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtConvex, PropertyInfo(Variant::ARRAY, "points"), set_points, get_points);
}

void BtConvex::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 500 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 200;
}

void BtConvex::set_points(const Vector<Vector3> &p_points) {
	point_count = p_points.size();
	if (point_count == 0 && points != nullptr) {
		memdelete(points);
		points = nullptr;
	} else {
		points = (btVector3 *)memrealloc(points, point_count * sizeof(btVector3));

		for (int i = 0; i < p_points.size(); i += 1) {
			G_TO_B(p_points[i], points[i]);
		}
	}

	update_internal_shape();
}

Vector<Vector3> BtConvex::get_points() const {
	Vector<Vector3> ret;
	ret.resize(point_count);
	Vector3 *ptrw = ret.ptrw();
	for (uint32_t i = 0; i < point_count; i += 1) {
		B_TO_G(points[i], ptrw[i]);
	}
	return ret;
}

void BtConvex::update_internal_shape() {
	const btVector3 local_scaling(1.0, 1.0, 1.0);
	if (point_count == 0) {
		convex.setPoints(nullptr, 0, true, local_scaling);
	} else {
		convex.setPoints(points, point_count, true, local_scaling);
	}
}

void BtTrimesh::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtTrimesh, PropertyInfo(Variant::ARRAY, "faces"), set_faces, get_faces);
}

void BtTrimesh::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 500 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 200;
}

void BtTrimesh::set_faces(const Vector<Vector3> &p_faces) {
	faces = p_faces;

	delete trimesh;
	trimesh = nullptr;

	mesh_interface = btTriangleMesh();

	if (p_faces.size() > 0) {
		// It counts the faces and assert the array contains the correct number of vertices.
		ERR_FAIL_COND_MSG((p_faces.size() % 3) != 0, "The sent arrays doesn't contains faces because the sent array is not a multiple of 3.");

		const int face_count = p_faces.size() / 3;

		mesh_interface.preallocateVertices(p_faces.size());

		const bool remove_duplicate = false;

		const Vector3 *facesr = p_faces.ptr();

		btVector3 supVec_0;
		btVector3 supVec_1;
		btVector3 supVec_2;
		for (int i = 0; i < face_count; i += 1) {
			G_TO_B(facesr[i * 3 + 0], supVec_0);
			G_TO_B(facesr[i * 3 + 1], supVec_1);
			G_TO_B(facesr[i * 3 + 2], supVec_2);

			// Inverted from standard godot otherwise btGenerateInternalEdgeInfo
			// generates wrong edge info.
			mesh_interface.addTriangle(supVec_2, supVec_1, supVec_0, remove_duplicate);
		}

		// Using `new` because Bullet Physics doesn't allow this to be constructed
		// elsewhere. Some data can't be set, so it's necessary set this at
		// constructor time.
		trimesh = new btBvhTriangleMeshShape(&mesh_interface, true);

		// Generate info map for better collision report.
		btGenerateInternalEdgeInfo(trimesh, &triangle_info_map);
	}
}

Vector<Vector3> BtTrimesh::get_faces() const {
	return faces;
}
