#pragma once

#include "../../databags/databag.h"
#include "core/templates/paged_allocator.h"
#include "shape_base.h"
#include <BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>

struct BtTrimesh : public BtRigidShape {
	COMPONENT_CUSTOM_CONSTRUCTOR(BtTrimesh, SharedSteadyStorage)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	Vector<Vector3> faces;
	btTriangleMesh mesh_interface;
	btTriangleInfoMap triangle_info_map;
	btBvhTriangleMeshShape trimesh = btBvhTriangleMeshShape(&mesh_interface, true, false);

	BtTrimesh() :
			BtRigidShape(TYPE_TRIMESH) {}

	void set_faces(const Vector<Vector3> &p_faces);
	Vector<Vector3> get_faces() const;

	ShapeInfo *add_shape(btScaledBvhTriangleMeshShape *p_shape, const Vector3 &p_scale);
};

struct BtShapeStorageTrimesh : public godex::Databag {
	DATABAG(BtShapeStorageTrimesh);

	PagedAllocator<btScaledBvhTriangleMeshShape, false> allocator;

	btCollisionShape *construct_shape(BtTrimesh *p_shape_owner, const Vector3 &p_scale);
};
