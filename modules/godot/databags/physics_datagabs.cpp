
#include "physics_datagabs.h"

#include "servers/physics_server_3d.h"

void Physics3D::_bind_methods() {
	//ECS_BIND_PROPERTY(Physics3D, PropertyInfo(Variant::OBJECT, "server"), server);
}

Physics3D::Physics3D() {
	server = PhysicsServer3D::get_singleton();
}

const PhysicsServer3D *Physics3D::get_server() const {
	return server;
}

PhysicsServer3D *Physics3D::get_server() {
	return server;
}
