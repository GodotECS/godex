#pragma once

#include "../../components/component.h"
#include "../../storage/dense_vector_storage.h"
#include "../../storage/shared_steady_storage.h"

class btCollisionShape;

struct BtRigidShape {
	enum ShapeType {
		TYPE_BOX,
		TYPE_SPHERE,
		TYPE_CAPSULE,
		TYPE_CONE,
		TYPE_CYLINDER,
		TYPE_WORLD_MARGIN,
		TYPE_CONVEX,
		TYPE_TRIMESH,
		TYPE_SHAPE_CONTAINER
	};

	struct ShapeInfo {
		btCollisionShape *shape_ptr;
		Vector3 scale;
	};

protected:
	/// This is used to know the type of the shape, so we know how to extract
	/// the shape common info, like the `btCollisionShape`.
	ShapeType type;

	LocalVector<ShapeInfo> shapes_info;

public:
	BtRigidShape(ShapeType p_type) :
			type(p_type) {}

	ShapeInfo *get_shape(const Vector3 &p_scale = Vector3(1, 1, 1));
	const ShapeInfo *get_shape(const Vector3 &p_scale = Vector3(1, 1, 1)) const;

	bool fallback_empty() const;

protected:
	ShapeInfo *__add_shape(btCollisionShape *p_shape, const Vector3 &p_scale);
};

/// The `BtStreamedShape` is a special shape that allow any system
/// to change the body shape with another already allocated elsewhere.
/// This is a lot useful for things like the `Pawn`, which changes
/// a lot.
struct BtStreamedShape : public BtRigidShape {
	COMPONENT_CUSTOM_CONSTRUCTOR(BtStreamedShape, DenseVectorStorage)

	btCollisionShape *shape = nullptr;

	BtStreamedShape() :
			BtRigidShape(TYPE_SHAPE_CONTAINER) {}
};
