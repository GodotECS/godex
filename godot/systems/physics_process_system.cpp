#include "physics_process_system.h"

#include "../../pipeline/pipeline.h"
#include "../resources/godot_engine_resources.h"
#include "core/config/engine.h"
#include "core/object/message_queue.h"
#include "core/os/os.h"

void Physics3DServerResource::_bind_properties() {
}

Physics3DServerResource::Physics3DServerResource() {
	physics_singleton = PhysicsServer3D::get_singleton();
}

PhysicsServer3D *Physics3DServerResource::get_physics() {
	return physics_singleton;
}

void call_physics_process(
		World *p_world,
		FrameTimeResource *p_iterator_info,
		Physics3DServerResource *p_physics_3d,
		EngineResource *p_engine,
		OsResource *p_os,
		MessageQueueResource *p_message_queue) {
	CRASH_COND_MSG(p_world == nullptr, "This is never nullptr because the `World` resource is automatically created by the `World` itself");
	ERR_FAIL_COND_MSG(p_iterator_info == nullptr, "The FrameTimeResource is not part of this world. Add it to use the physics.");
	ERR_FAIL_COND_MSG(p_physics_3d == nullptr, "The Physics3DServerResource is not part of this world. Add it to use the physics.");
	ERR_FAIL_COND_MSG(p_engine == nullptr, "The EngineResource is not part of this world. Add it to use the physics.");
	ERR_FAIL_COND_MSG(p_os == nullptr, "The OsResource is not part of this world. Add it to use the physics.");
	ERR_FAIL_COND_MSG(p_message_queue == nullptr, "The MessageQueueResource is not part of this world. Add it to use the physics.");

	const float physics_delta = p_iterator_info->get_physics_delta();

	// Make sure to update Godot nodes Signals and Transforms.
	p_physics_3d->get_physics()->flush_queries();

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

	const FrameTimeResource *godot_iterator = p_world->get_resource<FrameTimeResource>();
	ERR_FAIL_COND_MSG(godot_iterator == nullptr, "The Resource `FrameTimeResource` is not supposed to be null when using the `PhysicsPipelineDispatcher` `System`.");

	EngineResource *engine = p_world->get_resource<EngineResource>();
	ERR_FAIL_COND_MSG(engine == nullptr, "The Resource `EngineResource` is not supposed to be null when using the `PhysicsPipelineDispatcher` `System`.");

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
	r_info->with_resource(FrameTimeResource::get_resource_id(), false);
	r_info->with_resource(EngineResource::get_resource_id(), true);
	// Don't define any pipeline yet.
	r_info->set_pipeline(nullptr);
}

void step_physics_server_3d(
		const FrameTimeResource *p_iterator_info,
		Physics3DServerResource *p_physics,
		EngineResource *p_engine) {
	ERR_FAIL_COND_MSG(p_iterator_info == nullptr, "The FrameTimeResource is not part of this world. Add it to use the physics.");
	ERR_FAIL_COND_MSG(p_physics == nullptr, "The Physics3DServerResource is not part of this world. Add it to use the physics.");
	ERR_FAIL_COND_MSG(p_engine == nullptr, "The EngineResource is not part of this world. Add it to use the physics.");

	const float physics_delta = p_iterator_info->get_physics_delta();

	// Step the physics server.
	p_physics->get_physics()->step(physics_delta);

	p_engine->get_engine()->set_physics_frames(p_engine->get_engine()->get_physics_frames() + 1);
}
