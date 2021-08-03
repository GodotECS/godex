#include "shape_convex.h"

#include "bullet_types_converter.h"

void BtConvex::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtConvex, PropertyInfo(Variant::ARRAY, "points"), set_points, get_points);
}

void BtConvex::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 200 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 200;
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

	return *this;
}

BtConvex::~BtConvex() {
	if (points != nullptr) {
		memdelete(points);
	}
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
	update_shapes();
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

BtRigidShape::ShapeInfo *BtConvex::add_shape(btConvexPointCloudShape *p_shape, const Vector3 &p_scale) {
	btVector3 scale;
	G_TO_B(p_scale, scale);
	p_shape->setPoints(points, point_count, true, scale);
	return __add_shape(p_shape, p_scale);
}

void BtConvex::update_shapes() {
	for (uint32_t i = 0; i < shapes_info.size(); i += 1) {
		btConvexPointCloudShape *shape = static_cast<btConvexPointCloudShape *>(shapes_info[i].shape_ptr);
		btVector3 scale;
		G_TO_B(shapes_info[i].scale, scale);
		shape->setPoints(points, point_count, true, scale);
	}
}

btCollisionShape *BtShapeStorageConvex::construct_shape(BtConvex *p_shape_owner, const Vector3 &p_scale) {
	auto shape = allocator.alloc();
	p_shape_owner->add_shape(shape, p_scale);
	return shape;
}
