#include "shape_trimesh.h"

#include "bullet_types_converter.h"
#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>

void BtTrimesh::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtTrimesh, PropertyInfo(Variant::ARRAY, "faces"), set_faces, get_faces);
}

void BtTrimesh::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 200 Physis Bodies
	/// You can tweak this in editor.
	r_config["page_size"] = 200;
}

void BtTrimesh::set_faces(const Vector<Vector3> &p_faces) {
	faces = p_faces;

	// Update the mesh interfaces
	mesh_interface = btTriangleMesh();

	if (faces.size() > 0) {
		// It counts the faces and assert the array contains the correct number of vertices.
		ERR_FAIL_COND_MSG((faces.size() % 3) != 0, "The sent arrays doesn't contains faces because the sent array is not a multiple of 3.");

		const int face_count = p_faces.size() / 3;

		mesh_interface.preallocateVertices(p_faces.size());

		// TODO put true here?
		const bool remove_duplicate = false;

		const Vector3 *facesr = faces.ptr();

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

		trimesh = btBvhTriangleMeshShape(&mesh_interface, true, false);
		CRASH_COND_MSG(trimesh.getMeshInterface() != &mesh_interface, "This can't happen, since the `trimesh` is initialized properly.");
		trimesh.buildOptimizedBvh();
		trimesh.recalcLocalAabb();

		// Generate info map for better collision report.
		btGenerateInternalEdgeInfo(&trimesh, &triangle_info_map);
	}

	// Propagate the changes.
	// Nothing to propagate, everything is updated automatically.
}

Vector<Vector3> BtTrimesh::get_faces() const {
	return faces;
}

BtRigidShape::ShapeInfo *BtTrimesh::add_shape(btScaledBvhTriangleMeshShape *p_shape, const Vector3 &p_scale) {
	btVector3 scale;
	G_TO_B(p_scale, scale);
	p_shape->setLocalScaling(scale);
	return __add_shape(p_shape, p_scale);
}

btCollisionShape *BtShapeStorageTrimesh::construct_shape(BtTrimesh *p_shape_owner, const Vector3 &p_scale) {
	auto shape = allocator.alloc(&p_shape_owner->trimesh, btVector3(1.0, 1.0, 1.0));
	p_shape_owner->add_shape(shape, p_scale);
	return shape;
}
