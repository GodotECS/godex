#ifndef PHYSICS_PROCESS_SYSTEM_H
#define PHYSICS_PROCESS_SYSTEM_H

#include "../resources/ecs_resource.h"
#include "dynamic_system.h"
#include "servers/physics_server_3d.h"

class GodotIteratorInfoResource;
class PhysicsServer3D;
class EngineResource;
class OsResource;
class MessageQueueResource;

class Physics3DServerResource : public godex::Resource {
	RESOURCE(Physics3DServerResource)

	static void _bind_properties();

	PhysicsServer3D *physics_singleton;

public:
	Physics3DServerResource();

	PhysicsServer3D *get_physics();
};

void call_physics_process(
		GodotIteratorInfoResource *p_iterator_info,
		Physics3DServerResource *p_physics,
		EngineResource *p_engine,
		OsResource *p_os,
		MessageQueueResource *p_message_queue);

/// Creates a dynamic system where
void create_physics_system_dispatcher(godex::DynamicSystemInfo &r_info);

// ~~ 3D ~~
void step_physics_server_3d(
		const GodotIteratorInfoResource *p_iterator_info,
		Physics3DServerResource *p_physics,
		EngineResource *p_engine);

// ~~ 2D ~~
// TODO

#endif // PHYSICS_PROCES_SSYSTEM_H
