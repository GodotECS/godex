#include "physics_process_system.h"

#include "../resources/godot_engine_resources.h"
#include "core/config/engine.h"
#include "core/object/message_queue.h"
#include "core/os/os.h"

void physics_2d_process_system() {
	//ERR_FAIL_MSG("TODO implement physics process 2D.");
}

void Physics3DServerResource::_bind_properties() {
}

Physics3DServerResource::Physics3DServerResource() {
	physics_singleton = PhysicsServer3D::get_singleton();
}

PhysicsServer3D *Physics3DServerResource::get_physics() {
	return physics_singleton;
}

void physics_3d_process_system(
		GodotIteratorInfoResource *p_iterator_info,
		Physics3DServerResource *p_physics,
		EngineResource *p_engine,
		OsResource *p_os,
		MessageQueueResource *p_message_queue) {
	ERR_FAIL_COND_MSG(p_iterator_info == nullptr, "The GodotIteratorInfoResource is not part of this world. Add it to use the physics.");
	ERR_FAIL_COND_MSG(p_physics == nullptr, "The Physics3DServerResource is not part of this world. Add it to use the physics.");
	ERR_FAIL_COND_MSG(p_engine == nullptr, "The EngineResource is not part of this world. Add it to use the physics.");
	ERR_FAIL_COND_MSG(p_os == nullptr, "The OsResource is not part of this world. Add it to use the physics.");
	ERR_FAIL_COND_MSG(p_message_queue == nullptr, "The MessageQueueResource is not part of this world. Add it to use the physics.");

	const float frame_slice = p_iterator_info->get_frame_slice();
	const float time_scale = p_iterator_info->get_time_scale();

	p_engine->get_engine()->set_in_physics_frame(true);

	for (int i = 0; i < p_iterator_info->get_main_frame_time().physics_steps; i += 1) {
		p_physics->get_physics()->flush_queries();

		// TODO put here transform dispatching

		// Call `_physics_process`
		if (p_os->get_os()->get_main_loop()->iteration(frame_slice * time_scale)) {
			p_iterator_info->set_exit(true);
			break;
		}

		// TODO Flush the queue. This may be bad for multithread. Consider remove this.
		p_message_queue->get_queue()->flush();

		// Step the physics server.
		p_physics->get_physics()->step(frame_slice * time_scale);
	}

	p_engine->get_engine()->set_physics_frames(p_engine->get_engine()->get_physics_frames() + 1);
	p_engine->get_engine()->set_in_physics_frame(false);
}
