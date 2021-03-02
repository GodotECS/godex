
#pragma once

#include "../../../databags/databag.h"

class PhysicsServer3D;

/// `Physics3D` databag, allow to safely access the `PhysicsServer3D` from a
/// `System`.
class Physics3D : public godex::Databag {
	DATABAG(Physics3D)

	static void _bind_methods();

	PhysicsServer3D *server = nullptr;

public:
	Physics3D();

	const PhysicsServer3D *get_server() const;
	PhysicsServer3D *get_server();
};
