#ifndef PHYSICS_PROCESS_SYSTEM_H
#define PHYSICS_PROCESS_SYSTEM_H

#include "../../databags/databag.h"
#include "../../systems/dynamic_system.h"
#include "servers/physics_server_3d.h"

class FrameTime;
class PhysicsServer3D;
class EngineDatabag;
class OsDatabag;
class MessageQueueDatabag;

class Physics3DServerDatabag : public godex::Databag {
	DATABAG(Physics3DServerDatabag)

	static void _bind_properties();

	PhysicsServer3D *physics_singleton;

public:
	Physics3DServerDatabag();

	PhysicsServer3D *get_physics();
};

void call_physics_process(
		// This is needed to force process this system in single thread so that
		// `_physics_process()` can run safely.
		World *p_world,
		FrameTime *p_iterator_info,
		Physics3DServerDatabag *p_physics,
		EngineDatabag *p_engine,
		OsDatabag *p_os,
		MessageQueueDatabag *p_message_queue);

/// Creates a dynamic system where
void create_physics_system_dispatcher(godex::DynamicSystemInfo *r_info);

// ~~ 3D ~~
void step_physics_server_3d(
		const FrameTime *p_iterator_info,
		Physics3DServerDatabag *p_physics,
		EngineDatabag *p_engine);

// ~~ 2D ~~
// TODO

#endif // PHYSICS_PROCES_SSYSTEM_H
