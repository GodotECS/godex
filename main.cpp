
/**
	@author AndreaCatania
*/

#include "main/main.h"

#include "core/object/message_queue.h"
#include "ecs.h"
#include "main/main_timer_sync.h"

// TODO set fixed fps.
static const int fixed_fps = -1;

bool Main::custom_iteration(MainFrameTime *p_frame_time) {
	MessageQueue::get_singleton()->flush();

	bool exit = ECS::get_singleton()->dispatch_active_world();

	return exit;
}
