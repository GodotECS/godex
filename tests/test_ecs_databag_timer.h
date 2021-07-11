#ifndef TEST_ECS_DATABAG_TIMER_H
#define TEST_ECS_DATABAG_TIMER_H

#include "tests/test_macros.h"

#include "../modules/godot/databags/databag_timer.h"
#include "../world/world.h"

namespace godex_tests {

TEST_CASE("[Modules][ECS] Test timer databag operations.") {
	// Test TimerHandle to uint64_t and inverse conversions used mainly on gdscript interface
	{
		uint32_t testCases[18][2] = { { 0, 0 }, { UINT32_MAX, UINT32_MAX }, { UINT32_MAX, 0 }, { 0, UINT32_MAX }, { UINT32_MAX - 42, UINT32_MAX / 42 }, { UINT32_MAX, 8 }, { 16, UINT32_MAX }, { 11, 0 }, { 0, 6 }, { 1993, 28 }, { 2863311530, 2863311530 }, { 1431655765, 1431655765 }, { 2863311530, 1431655765 }, { 1431655765, 2863311530 }, { 2863289685, 2863289685 }, { 1431677610, 1431677610 }, { 2863289685, 1431677610 }, { 1431677610, 2863289685 } };
		for (long unsigned int i = 0; i < sizeof(testCases) / sizeof(*testCases); i++) {
			const godex::TimerIndex ai = testCases[i][0];
			const godex::TimerGeneration ag = testCases[i][1];
			godex::TimerHandle ah = godex::TimerHandle(ai, ag);
			uint64_t au = static_cast<uint64_t>(ah);
			godex::TimerHandle ac = static_cast<godex::TimerHandle>(au);
			godex::TimerHandle ar = godex::TimerHandle(au);
			CHECK(ac.timer_index == ai);
			CHECK(ac.generation == ag);
			CHECK(ar.timer_index == ai);
			CHECK(ar.generation == ag);
		}
	}

	// Test basic timer databag operations
	{
		TimersDatabag td;

		godex::TimerHandle handle = td.new_precise_timer(1.0 * 1'000'000.0);
		CHECK(td.is_valid_timer_handle(handle));
		CHECK(td.is_done(handle) == false);
		CHECK(td.get_remaining_seconds(handle) >= (1.0 - CMP_EPSILON));

		td.internal_set_now(0.5 * 1'000'000.0);
		CHECK(td.get_remaining_seconds(handle) >= (0.5 - CMP_EPSILON));

		td.internal_set_now(2 * 1'000'000.0);
		CHECK(td.is_done(handle));

		td.destroy_timer(handle);
		CHECK(td.is_valid_timer_handle(handle) == false);

		handle = td.new_timer(1.0);
		CHECK(td.is_valid_timer_handle(handle));
		CHECK(td.is_done(handle) == false);
		CHECK(td.get_remaining_seconds(handle) >= (1.0 - CMP_EPSILON));

		td.internal_set_now(2.5 * 1'000'000.0);
		CHECK(td.get_remaining_seconds(handle) >= (0.5 - CMP_EPSILON));

		td.internal_set_now(4 * 1'000'000.0);
		CHECK(td.is_done(handle));
	}

	// Test basic global pause timer databag operations
	{
		TimersDatabag td;

		godex::TimerHandle handle = td.new_precise_timer(1.0 * 1'000'000.0);

		bool paused = true;
		uint64_t old_now = td.internal_get_full_now();
		uint64_t old_paused_time = td.internal_get_pause();
		uint64_t new_now = old_now + 1.5 * 1'000'000.0;

		// this is only a substitute of what the timer updater system would be doing.
		td.internal_set_now(new_now);
		if (paused) {
			td.internal_set_pause(old_paused_time + (new_now - old_now));
		}
		// End of substitute

		CHECK(td.is_done(handle) == false);
		CHECK(td.get_remaining_seconds(handle) >= (1.0 - CMP_EPSILON));

		paused = false;
		old_now = td.internal_get_full_now();
		old_paused_time = td.internal_get_pause();
		new_now = old_now + 1.5 * 1'000'000.0;

		// this is only a substitute of what the timer updater system would be doing.
		td.internal_set_now(new_now);
		if (paused) {
			td.internal_set_pause(old_paused_time + (new_now - old_now));
		}
		// End of substitute

		CHECK(td.is_done(handle));
		CHECK(td.get_remaining_seconds(handle) >= (-0.5 - CMP_EPSILON));
	}
}
} // namespace godex_tests

#endif // TEST_ECS_DATABAG_TIMER_H
