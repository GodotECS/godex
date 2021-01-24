#include "physics_process_system.h"

#include "../../pipeline/pipeline.h"
#include "../databags/godot_engine_databags.h"
#include "core/config/engine.h"
#include "core/object/message_queue.h"
#include "core/os/os.h"

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
}

void physics_pipeline_dispatcher(World *p_world, Pipeline *p_pipeline) {
	ERR_FAIL_COND_MSG(p_pipeline == nullptr, "The `PhysicsPipelineDispatcher` doesn't have a pipeline assigned, so it's doing nothing. Assign it please.");

	const FrameTime *godot_iterator = p_world->get_databag<FrameTime>();
	ERR_FAIL_COND_MSG(godot_iterator == nullptr, "The Databag `FrameTimeDatabag` is not supposed to be null when using the `PhysicsPipelineDispatcher` `System`.");

	EngineDatabag *engine = p_world->get_databag<EngineDatabag>();
	ERR_FAIL_COND_MSG(engine == nullptr, "The Databag `EngineDatabag` is not supposed to be null when using the `PhysicsPipelineDispatcher` `System`.");

	engine->get_engine()->set_in_physics_frame(true);

	for (int i = 0; i < godot_iterator->get_main_frame_time().physics_steps; i += 1) {
		// Dispatches the sub pipeline.
		p_pipeline->dispatch(p_world);

		if (unlikely(godot_iterator->get_exit())) {
			// If at this point the exit is true just end the processing, to
			// shut down the engine as soon as possible.
			return;
		}
	}

	engine->get_engine()->set_in_physics_frame(false);
}

void create_physics_system_dispatcher(godex::DynamicSystemInfo *r_info) {
	r_info->set_target(physics_pipeline_dispatcher);
	r_info->with_databag(FrameTime::get_databag_id(), false);
	r_info->with_databag(EngineDatabag::get_databag_id(), true);
	// Don't define any pipeline yet.
	r_info->set_pipeline(nullptr);
	// Build.
	r_info->build();
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
