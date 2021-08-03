#include "physics_process_system.h"

#include "../../../pipeline/pipeline.h"
#include "../../../storage/hierarchical_storage.h"
#include "../databags/godot_engine_databags.h"
#include "core/config/engine.h"
#include "core/object/message_queue.h"
#include "core/os/os.h"

uint32_t physics_pipeline_dispatcher(const FrameTime *p_frame_time) {
	return p_frame_time->get_main_frame_time().physics_steps;
}

void physics_init_frame(EngineDatabag *p_engine) {
	p_engine->get_engine()->set_in_physics_frame(true);
}

void physics_finalize_frame(EngineDatabag *p_engine) {
	p_engine->get_engine()->set_in_physics_frame(false);
}

void call_physics_process(
		World *p_world,
		FrameTime *p_iterator_info,
		Physics3D *p_physics_3d,
		EngineDatabag *p_engine,
		OsDatabag *p_os,
		MessageQueueDatabag *p_message_queue) {
	CRASH_COND_MSG(p_world == nullptr, "This is never nullptr because the `World` databag is automatically created by the `World` itself");
	ERR_FAIL_COND_MSG(p_iterator_info == nullptr, "The FrameTimeDatabag is not part of this world. Add it to use the physics.");
	ERR_FAIL_COND_MSG(p_physics_3d == nullptr, "The Physics3DServerDatabag is not part of this world. Add it to use the physics.");
	ERR_FAIL_COND_MSG(p_engine == nullptr, "The EngineDatabag is not part of this world. Add it to use the physics.");
	ERR_FAIL_COND_MSG(p_os == nullptr, "The OsDatabag is not part of this world. Add it to use the physics.");
	ERR_FAIL_COND_MSG(p_message_queue == nullptr, "The MessageQueueDatabag is not part of this world. Add it to use the physics.");

	const float physics_delta = p_iterator_info->get_physics_delta();

	// Make sure to update Godot nodes Signals and Transforms.
	p_physics_3d->get_server()->flush_queries();

	// TODO put 2D flush_query and sync here.

	// Call `_physics_process`
	if (p_os->get_os()->get_main_loop()->physics_process(physics_delta)) {
		p_iterator_info->set_exit(true);
		return;
	}

	// TODO Flush the queue. This may be bad for multithread. Consider remove
	// this or put it in its own System.
	p_message_queue->get_queue()->flush();

	p_engine->get_engine()->set_physics_frames(p_engine->get_engine()->get_physics_frames() + 1);

	// Flush any hierarchy change. GDScript may have changed it.
	// Trust this that `Child` is using the `Hierarchy` storage.
	Hierarchy *hierarchy = static_cast<Hierarchy *>(p_world->get_storage<Child>());
	if (hierarchy) {
		hierarchy->flush_hierarchy_changes();
	}
}

void step_physics_server_3d(
		const FrameTime *p_iterator_info,
		Physics3D *p_physics,
		EngineDatabag *p_engine) {
	ERR_FAIL_COND_MSG(p_iterator_info == nullptr, "The FrameTimeDatabag is not part of this world. Add it to use the physics.");
	ERR_FAIL_COND_MSG(p_physics == nullptr, "The Physics3DServerDatabag is not part of this world. Add it to use the physics.");
	ERR_FAIL_COND_MSG(p_engine == nullptr, "The EngineDatabag is not part of this world. Add it to use the physics.");

	const float physics_delta = p_iterator_info->get_physics_delta();

	// Step the physics server.
	p_physics->get_server()->step(physics_delta);

	p_engine->get_engine()->set_physics_frames(p_engine->get_engine()->get_physics_frames() + 1);
}
