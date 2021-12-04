#include "databag_timer.h"

void TimersDatabag::_bind_methods() {
	add_method("is_valid_timer_handle", &TimersDatabag::script_is_valid_timer_handle);
	add_method("new_timer", &TimersDatabag::script_new_timer);
	add_method("new_precise_timer", &TimersDatabag::script_new_precise_timer);
	add_method("restart_timer", &TimersDatabag::script_restart_timer);
	add_method("restart_precise_timer", &TimersDatabag::script_restart_precise_timer);
	add_method("is_done", &TimersDatabag::script_is_done);
	add_method("destroy_timer", &TimersDatabag::script_destroy_timer);
	add_method("get_remaining_seconds", &TimersDatabag::script_get_remaining_seconds);
	add_method("get_remaining_microseconds", &TimersDatabag::script_get_remaining_microseconds);
}

void TimersDatabag::internal_set_now(const uint64_t p_now) {
	now = p_now;
}

void TimersDatabag::internal_set_pause(const uint64_t p_new_pause) {
	paused_time = p_new_pause;
}

bool TimersDatabag::is_valid_timer_handle(const godex::TimerHandle p_timer_handle) const {
	return p_timer_handle.timer_index < timers.size() && timers[p_timer_handle.timer_index].generation == p_timer_handle.generation && timers[p_timer_handle.timer_index].generation > 0;
}

godex::TimerHandle TimersDatabag::new_timer(const real_t p_end_in_seconds) {
	ERR_FAIL_COND_V_MSG(p_end_in_seconds < 0, godex::TimerHandle(), "Can't make a new timer that ends in the past.");
	return new_precise_timer(static_cast<uint64_t>(p_end_in_seconds * 1'000'000.0));
}

godex::TimerHandle TimersDatabag::new_precise_timer(const uint64_t p_end_in_microseconds) {
	if (destroyed_timers.size() > 0) {
		godex::TimerHandle finalHandle = destroyed_timers[destroyed_timers.size() - 1];
		internal_restart_precise_timer(finalHandle, internal_get_now() + p_end_in_microseconds);
		destroyed_timers.remove_at_unordered(destroyed_timers.size() - 1);
		return finalHandle;
	} else {
		timers.push_back(godex::Timer(internal_get_now() + p_end_in_microseconds, 1));
		return godex::TimerHandle(timers.size() - 1, 1);
	}
}

void TimersDatabag::restart_timer(const godex::TimerHandle p_timer_handle, const real_t p_end_in_seconds) {
	ERR_FAIL_COND_MSG(p_end_in_seconds < 0, "Can't make a new timer that ends in the past.");
	restart_precise_timer(p_timer_handle, static_cast<uint64_t>(p_end_in_seconds * 1'000'000.0));
}

void TimersDatabag::restart_precise_timer(const godex::TimerHandle p_timer_handle, const uint64_t p_end_in_microseconds) {
	ERR_FAIL_COND_MSG(!is_valid_timer_handle(p_timer_handle), "The handle" + itos(p_timer_handle.timer_index) + " is not a valid handle.");
	internal_restart_precise_timer(p_timer_handle, p_end_in_microseconds);
}

bool TimersDatabag::is_done(const godex::TimerHandle p_timer_handle) const {
	ERR_FAIL_COND_V_MSG(!is_valid_timer_handle(p_timer_handle), true, "The handle" + itos(p_timer_handle.timer_index) + " is not a valid handle.");
	return timers[p_timer_handle.timer_index].end_time <= internal_get_now();
}

void TimersDatabag::destroy_timer(const godex::TimerHandle p_timer_handle) {
	if (is_valid_timer_handle(p_timer_handle)) {
		destroyed_timers.push_back(p_timer_handle);
		destroyed_timers[destroyed_timers.size() - 1].generation++;
		timers[p_timer_handle.timer_index].end_time = 0;
		timers[p_timer_handle.timer_index].generation++;
	}
}

real_t TimersDatabag::get_remaining_seconds(const godex::TimerHandle p_timer_handle) const {
	return (is_done(p_timer_handle) ? -static_cast<double>(internal_get_now() - timers[p_timer_handle.timer_index].end_time) : static_cast<double>(timers[p_timer_handle.timer_index].end_time - internal_get_now())) / 1'000'000.0;
}

int64_t TimersDatabag::get_remaining_microseconds(const godex::TimerHandle p_timer_handle) const {
	return is_done(p_timer_handle) ? -static_cast<int64_t>(internal_get_now() - timers[p_timer_handle.timer_index].end_time) : static_cast<int64_t>(timers[p_timer_handle.timer_index].end_time - internal_get_now());
}
