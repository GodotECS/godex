#include "timer_updater_system.h"
#include "core/os/os.h"
#include "scene/main/scene_tree.h"

void timer_updater_system(TimersDatabag *td,
		const SceneTreeInfoDatabag *p_sti,
		const OsDatabag *p_os) {
	const bool paused = p_sti->is_paused();
	const uint64_t old_now = td->internal_get_full_now();
	const uint64_t old_paused_time = td->internal_get_pause();
	const uint64_t new_now = p_os->get_os()->get_ticks_usec();
	ERR_FAIL_COND_MSG(new_now < old_now, "Travel only forward through our timeline, please.");

	td->internal_set_now(new_now);
	if (paused) {
		ERR_FAIL_COND_MSG(old_paused_time > new_now, "Somehow we ended up spending more time paused than total play-time. This should not happen.");
		td->internal_set_pause(old_paused_time + (new_now - old_now));
	}
}

void timer_event_launcher_system(TimersDatabag *td) {
	// TODO: Implement using the new event system
}
