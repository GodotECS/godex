
/**
	@author AndreaCatania
*/

#include "main/main.h"

#include "core/object/message_queue.h"
#include "ecs.h"
#include "godot/resources/godot_engine_resources.h"
#include "main/main_timer_sync.h"
#include "world/world.h"

bool Main::custom_iteration(float p_idle_delta, class MainFrameTime *p_frame_time, float p_physics_frame_slice, float p_time_scale) {
	MessageQueue::get_singleton()->flush();

	World *w = ECS::get_singleton()->get_active_world();
	if (likely(w)) {
		FrameTimeResource *info = w->get_resource<FrameTimeResource>();
		if (likely(info)) {
			info->set_main_frame_time(*p_frame_time);
			info->set_delta(p_idle_delta * p_time_scale);
			info->set_physics_delta(p_physics_frame_slice * p_time_scale);
		}
	}

	ECS::get_singleton()->dispatch_active_world();

	if (likely(w)) {
		FrameTimeResource *info = w->get_resource<FrameTimeResource>();
		if (likely(info)) {
			return info->get_exit();
		}
	}

	return false;
}
