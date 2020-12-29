
/**
	@author AndreaCatania
*/

#include "main/main.h"

#include "core/object/message_queue.h"
#include "ecs.h"
#include "godot/resources/godot_engine_resources.h"
#include "main/main_timer_sync.h"
#include "world/world.h"

bool Main::custom_iteration(MainFrameTime *p_frame_time, float p_frame_slice, float p_time_scale) {
	MessageQueue::get_singleton()->flush();

	World *w = ECS::get_singleton()->get_active_world();
	if (likely(w)) {
		GodotIteratorInfoResource *info = w->get_resource<GodotIteratorInfoResource>();
		if (likely(info)) {
			info->set_main_frame_time(*p_frame_time);
			info->set_frame_slice(p_frame_slice);
			info->set_time_scale(p_time_scale);
		}
	}

	ECS::get_singleton()->dispatch_active_world();

	if (likely(w)) {
		GodotIteratorInfoResource *info = w->get_resource<GodotIteratorInfoResource>();
		if (likely(info)) {
			return info->get_exit();
		}
	}

	return false;
}
