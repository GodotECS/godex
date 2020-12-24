#ifndef PHYSICS_PROCESS_SYSTEM_H
#define PHYSICS_PROCESS_SYSTEM_H

#include "../resources/ecs_resource.h"
#include "servers/physics_server_3d.h"

class GodotIteratorInfoResource;
class PhysicsServer3D;
class EngineResource;
class OsResource;
class MessageQueueResource;

void physics_2d_process_system();

class Physics3DServerResource : public godex::Resource {
	RESOURCE(Physics3DServerResource)

	static void _bind_properties();

	PhysicsServer3D *physics_singleton;

public:
	Physics3DServerResource();

	PhysicsServer3D *get_physics();
};

void physics_3d_process_system(
		GodotIteratorInfoResource *p_iterator_info,
		Physics3DServerResource *p_physics,
		EngineResource *p_engine,
		OsResource *p_os,
		MessageQueueResource *p_message_queue);

#endif // PHYSICS_PROCES_SSYSTEM_H
