#ifndef PHYSICS_PROCESS_SYSTEM_H
#define PHYSICS_PROCESS_SYSTEM_H

#include "../resources/ecs_resource.h"
#include "servers/physics_server_3d.h"

void physics_2d_process_system();

class Physics3DServerResource : public godex::Resource {
	RESOURCE(Physics3DServerResource)

	static void _bind_properties();

public:
	Physics3DServerResource();
};

void physics_3d_process_system(Physics3DServerResource *p_physics_resource);

#endif // PHYSICS_PROCES_SSYSTEM_H
