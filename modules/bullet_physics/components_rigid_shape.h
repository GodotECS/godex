#pragma once

#include "../../components/component.h"
#include "../../storage/dense_vector_storage.h"
#include "../../storage/shared_steady_storage.h"
#include <BulletCollision/CollisionShapes/btConvexPointCloudShape.h>
#include <btBulletCollisionCommon.h>

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

protected:
	/// This is used to know the type of the shape, so we know how to extract
	/// the shape common info, like the `btCollisionShape`.
	ShapeType type;

	/// List of bodies where this shape is assigned.
	/// TODO Do I really need this, can I take this infor from the storage instead??
	LocalVector<EntityID> bodies;

public:
	BtRigidShape(ShapeType p_type) :
			type(p_type) {}

	btCollisionShape *get_shape();
	const btCollisionShape *get_shape() const;

	bool fallback_empty() const;

	void add_body(EntityID p_entity) {
		bodies.push_back(p_entity);
	}
};

struct BtBox : public BtRigidShape {
	COMPONENT_CUSTOM_CONSTRUCTOR(BtBox, SharedSteadyStorage)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	btBoxShape box = btBoxShape(btVector3(1.0, 1.0, 1.0));

	BtBox() :
			BtRigidShape(TYPE_BOX) {}

	void set_half_extents(const Vector3 &p_half_extends);
	Vector3 get_half_extents() const;

	void set_margin(real_t p_margin);
	real_t get_margin() const;
};

struct BtSphere : public BtRigidShape {
	COMPONENT_CUSTOM_CONSTRUCTOR(BtSphere, SharedSteadyStorage)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	btSphereShape sphere = btSphereShape(1.0);

	BtSphere() :
			BtRigidShape(TYPE_SPHERE) {}

	void set_radius(real_t p_radius);
	real_t get_radius() const;
};

struct BtCapsule : public BtRigidShape {
	COMPONENT_CUSTOM_CONSTRUCTOR(BtCapsule, SharedSteadyStorage)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	btCapsuleShape capsule = btCapsuleShape(1.0, 1.0); // Radius, Height

	BtCapsule() :
			BtRigidShape(TYPE_CAPSULE) {}

	void set_radius(real_t p_radius);
	real_t get_radius() const;

	void set_height(real_t p_height);
	real_t get_height() const;
};

struct BtCone : public BtRigidShape {
	COMPONENT_CUSTOM_CONSTRUCTOR(BtCone, SharedSteadyStorage)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	btConeShape cone = btConeShape(1.0, 1.0); // Radius, Height

	BtCone() :
			BtRigidShape(TYPE_CONE) {}

	void set_radius(real_t p_radius);
	real_t get_radius() const;

	void set_height(real_t p_height);
	real_t get_height() const;

	void set_margin(real_t p_margin);
	real_t get_margin() const;
};

struct BtCylinder : public BtRigidShape {
	COMPONENT_CUSTOM_CONSTRUCTOR(BtCylinder, SharedSteadyStorage)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	btCylinderShape cylinder = btCylinderShape(btVector3(1.0, 1.0, 1.0)); // Half extents

	BtCylinder() :
			BtRigidShape(TYPE_CYLINDER) {}

	void set_radius(real_t p_radius);
	real_t get_radius() const;

	void set_height(real_t p_height);
	real_t get_height() const;

	void set_margin(real_t p_margin);
	real_t get_margin() const;
};

struct BtWorldMargin : public BtRigidShape {
	COMPONENT_CUSTOM_CONSTRUCTOR(BtWorldMargin, SharedSteadyStorage)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	btStaticPlaneShape world_margin = btStaticPlaneShape(btVector3(0, 1, 0), 0.0); // Normal, Distance

	BtWorldMargin() :
			BtRigidShape(TYPE_WORLD_MARGIN) {}
};

struct BtConvex : public BtRigidShape {
	COMPONENT_CUSTOM_CONSTRUCTOR(BtConvex, SharedSteadyStorage)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	btVector3 *points = nullptr;
	uint32_t point_count = 0;
	btConvexPointCloudShape convex = btConvexPointCloudShape();

	BtConvex() :
			BtRigidShape(TYPE_CONVEX) {}
	// Copy constructor is needed because I'm dealing with pointers here.
	BtConvex(const BtConvex &p_other);
	BtConvex &operator=(const BtConvex &p_other);
	~BtConvex();

	void set_points(const Vector<Vector3> &p_points);
	Vector<Vector3> get_points() const;

	void update_internal_shape();
};

struct BtTrimesh : public BtRigidShape {
	COMPONENT_CUSTOM_CONSTRUCTOR(BtTrimesh, SharedSteadyStorage)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	Vector<Vector3> faces;
	btTriangleMesh mesh_interface;
	btTriangleInfoMap triangle_info_map;
	btBvhTriangleMeshShape *trimesh = nullptr;

	BtTrimesh() :
			BtRigidShape(TYPE_TRIMESH) {}

	void set_faces(const Vector<Vector3> &p_faces);
	Vector<Vector3> get_faces() const;
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
