#ifndef PHYSICS_PROCESS_SYSTEM_H
#define PHYSICS_PROCESS_SYSTEM_H

#include "../../../databags/frame_time.h"
#include "../../../systems/dynamic_system.h"
#include "../databags/physics_datagabs.h"
#include "servers/physics_server_3d.h"

class PhysicsServer3D;
class EngineDatabag;
class OsDatabag;
class MessageQueueDatabag;

uint32_t physics_pipeline_dispatcher(const FrameTime *p_frame_time);
void physics_init_frame(EngineDatabag *p_engine);
void physics_finalize_frame(EngineDatabag *p_engine);

void call_physics_process(
		// This is needed to force process this system in single thread so that
		// `_physics_process()` can run safely.
		World *p_world,
		FrameTime *p_iterator_info,
		Physics3D *p_physics,
		EngineDatabag *p_engine,
		OsDatabag *p_os,
		MessageQueueDatabag *p_message_queue);

// ~~ 3D ~~
void step_physics_server_3d(
		const FrameTime *p_iterator_info,
		Physics3D *p_physics,
		EngineDatabag *p_engine);

// ~~ 2D ~~
// TODO

#endif // PHYSICS_PROCES_SSYSTEM_H
