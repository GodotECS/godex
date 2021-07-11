#pragma once

#include "../../../databags/databag.h"
#include "core/math/math_defs.h"
#include "core/templates/local_vector.h"

namespace godex {
typedef uint32_t TimerIndex;

// Change to a bitfield in a struct if more data is needed for timer and timerHandle,
// or reduce to int16 if it's needed only on one of the two types, we don't need the
// full int 32 for the generation. Might as well use it until we find another purpose for
// this memory.
typedef uint32_t TimerGeneration;

struct Timer {
	Timer(uint64_t p_endTime, godex::TimerGeneration p_generation) :
			end_time(p_endTime), generation(p_generation) {}
	Timer() = default;

	// μs (microseconds) until the timer fires
	uint64_t end_time{ 0 };
	godex::TimerGeneration generation{ 0 };
};

struct TimerHandle {
	TimerHandle(godex::TimerIndex p_timerIndex, godex::TimerGeneration p_generation) :
			timer_index(p_timerIndex), generation(p_generation) {}
	TimerHandle() = default;
	TimerHandle(const uint64_t &p_timer_handle_int) :
			timer_index(p_timer_handle_int & UINT32_MAX), generation(p_timer_handle_int >> 32) {}

	explicit operator uint64_t() const {
		return static_cast<uint64_t>(generation) << 32 | static_cast<uint64_t>(timer_index);
	}

	// Index on the timers array of the referred timer
	godex::TimerIndex timer_index{ UINT32_MAX };
	godex::TimerGeneration generation{ 0 };
};
}; // Namespace godex

class TimersDatabag : public godex::Databag {
	DATABAG(TimersDatabag)

	static void _bind_methods();

	// In μs (microseconds)
	uint64_t now{ 0 };
	uint64_t paused_time{ 0 };
	LocalVector<godex::Timer, godex::TimerIndex> timers;
	LocalVector<godex::TimerHandle, godex::TimerIndex> destroyed_timers;

	// For internal usage only: has no safety checks.
	_FORCE_INLINE_ void internal_restart_precise_timer(const godex::TimerHandle p_timer_handle, const uint64_t p_microseconds) {
		timers[p_timer_handle.timer_index].end_time = p_microseconds;
	}

	// For internal usage only: has no safety checks.
	_FORCE_INLINE_ uint64_t internal_get_now() const {
		return now - paused_time;
	}

public:
	TimersDatabag() {}

	/// Used by `timer_updater_system` `System` to update time by setting a new current time.
	void internal_set_now(const uint64_t p_now);

	/// Used by `timer_updater_system` `System` to update paused time.
	void internal_set_pause(const uint64_t p_pause);

	/// Used by `timer_updater_system` `System` to retrieve paused time.
	_FORCE_INLINE_ uint64_t internal_get_pause() const {
		return paused_time;
	}

	/// Used by `timer_updater_system` `System` to retrieve current time without taking into account paused time.
	_FORCE_INLINE_ uint64_t internal_get_full_now() const {
		return now;
	}

	/// <summary>
	/// Check if the handle refers to the original timer and if it is still running.
	/// </summary>
	/// <param name="p_timer_handle"></param>
	/// <returns></returns>
	bool is_valid_timer_handle(const godex::TimerHandle p_timer_handle) const;

	/// <summary>
	/// Creates a new timer the provided amount of seconds from now.
	/// Returns the handle that identifies the new timer.
	/// </summary>
	/// <param name="p_end_in_seconds"></param>
	/// <returns></returns>
	godex::TimerHandle new_timer(const real_t p_end_in_seconds);

	/// <summary>
	/// Creates a new timer the provided amount of microseconds from now.
	/// Returns the handle that identifies the new timer.
	/// This version keeps the full precision of uint64_t
	/// </summary>
	/// <param name="p_end_in_microseconds"></param>
	/// <returns></returns>
	godex::TimerHandle new_precise_timer(const uint64_t p_end_in_microseconds);

	/// <summary>
	/// Set a new time for an existing timer.
	/// </summary>
	/// <param name="p_timer_handle"></param>
	/// <param name="p_end_in_seconds"></param>
	void restart_timer(const godex::TimerHandle p_timer_handle, const real_t p_end_in_seconds);

	/// <summary>
	/// Set a new time for an existing timer.
	/// This version keeps the full precision of uint64_t.
	/// </summary>
	/// <param name="p_timer_handle"></param>
	/// <param name="p_end_in_microseconds"></param>
	void restart_precise_timer(const godex::TimerHandle p_timer_handle, const uint64_t p_end_in_microseconds);

	/// <summary>
	/// Check if a valid timer has finished or is still set in the future.
	/// </summary>
	/// <param name="p_timer_handle"></param>
	/// <returns></returns>
	bool is_done(const godex::TimerHandle p_timer_handle) const;

	/// <summary>
	/// Invalidates and returns the timer to the memory pool.
	/// Safe no-op when called on already invalid timer handles.
	/// </summary>
	/// <param name="p_timer_handle"></param>
	void destroy_timer(const godex::TimerHandle p_timer_handle);

	/// <summary>
	/// Get the amount of seconds that remain until the timer ends.
	/// For ended timers, the negative of the amount of seconds that passed since it ended.
	/// </summary>
	/// <param name="p_timer_handle"></param>
	/// <returns></returns>
	real_t get_remaining_seconds(const godex::TimerHandle p_timer_handle) const;

	/// <summary>
	/// Get the amount of microseconds that remain until the timer ends.
	/// For ended timers, the negative of the amount of microseconds that passed since it ended.
	/// </summary>
	/// <param name="p_timer_handle"></param>
	/// <returns></returns>
	int64_t get_remaining_microseconds(const godex::TimerHandle p_timer_handle) const;

private:
	// Script only version of c++ api

	_FORCE_INLINE_ bool script_is_valid_timer_handle(const uint64_t p_timer_handle_int) const {
		return is_valid_timer_handle(static_cast<godex::TimerHandle>(p_timer_handle_int));
	}
	_FORCE_INLINE_ uint64_t script_new_timer(const real_t p_end_in_seconds) {
		return static_cast<uint64_t>(new_timer(p_end_in_seconds));
	}
	_FORCE_INLINE_ uint64_t script_new_precise_timer(const uint64_t p_end_in_microseconds) {
		return static_cast<uint64_t>(new_precise_timer(p_end_in_microseconds));
	}
	_FORCE_INLINE_ void script_restart_timer(const uint64_t p_timer_handle_int, const real_t p_end_in_seconds) {
		restart_timer(static_cast<godex::TimerHandle>(p_timer_handle_int), p_end_in_seconds);
	}
	_FORCE_INLINE_ void script_restart_precise_timer(const uint64_t p_timer_handle_int, const uint64_t p_end_in_microseconds) {
		restart_precise_timer(static_cast<godex::TimerHandle>(p_timer_handle_int), p_end_in_microseconds);
	}
	_FORCE_INLINE_ bool script_is_done(const uint64_t p_timer_handle_int) const {
		return is_done(static_cast<godex::TimerHandle>(p_timer_handle_int));
	}
	_FORCE_INLINE_ void script_destroy_timer(const uint64_t p_timer_handle_int) {
		destroy_timer(static_cast<godex::TimerHandle>(p_timer_handle_int));
	}
	_FORCE_INLINE_ real_t script_get_remaining_seconds(const uint64_t p_timer_handle_int) const {
		return get_remaining_seconds(static_cast<godex::TimerHandle>(p_timer_handle_int));
	}
	_FORCE_INLINE_ int64_t script_get_remaining_microseconds(const uint64_t p_timer_handle_int) const {
		return get_remaining_microseconds(static_cast<godex::TimerHandle>(p_timer_handle_int));
	}
};
