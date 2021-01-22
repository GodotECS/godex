#ifndef TEST_ECS_STARTUP_SYSTEM_H
#define TEST_ECS_STARTUP_SYSTEM_H

#include "tests/test_macros.h"

#include "../ecs.h"
#include "../pipeline/pipeline.h"
#include "../world/world.h"

struct ExecutionCounter : public godex::Databag {
	DATABAG(ExecutionCounter)

public:
	uint32_t prev = 0;
	uint32_t count = 0;
};

namespace godex_tests_startup_system {

bool startup_system_1_test(ExecutionCounter *p_counter) {
	p_counter->count += 1;
	return p_counter->count >= 2;
}

TEST_CASE("[Modules][ECS] Test startup system.") {
	ECS::register_databag<ExecutionCounter>();

	World world;
	world.add_databag<ExecutionCounter>();

	Pipeline pipeline;
	pipeline.add_startup_system(startup_system_1_test);
	pipeline.build();
	pipeline.prepare(&world);

	for (uint32_t i = 0; i < 10; i += 1) {
		pipeline.dispatch(&world);
	}

	const ExecutionCounter *counter = world.get_databag<ExecutionCounter>();
	// Make sure this run just twice.
	CHECK(counter->count == 2);
}

TEST_CASE("[Modules][ECS] Test registered startup system.") {
	ECS::register_startup_system(startup_system_1_test, "StartupSystemTest", "");

	World world;
	world.add_databag<ExecutionCounter>();

	Pipeline pipeline;
	pipeline.add_registered_startup_system(ECS::get_system_id("StartupSystemTest"));
	pipeline.build();
	pipeline.prepare(&world);

	for (uint32_t i = 0; i < 10; i += 1) {
		pipeline.dispatch(&world);
	}

	const ExecutionCounter *counter = world.get_databag<ExecutionCounter>();
	// Make sure this run just twice.
	CHECK(counter->count == 2);
}

bool startup_system_2_test(ExecutionCounter *p_counter) {
	p_counter->count += 1;

	if (p_counter->count == 5) {
		return true;
	}

	return false;
}

bool startup_system_3_test(ExecutionCounter *p_counter) {
	p_counter->count += 1;
	return true;
}

bool startup_system_4_test(ExecutionCounter *p_counter) {
	if (p_counter->count == 2) {
		// Make sure this is executed after `system_2`.
		CHECK(p_counter->prev == 0);
	} else {
		// Make sure this is executed after `system_3`.
		CHECK(p_counter->prev + 1 == p_counter->count);
	}

	p_counter->prev = p_counter->count;

	if (p_counter->count == 5) {
		return true;
	}

	return false;
}

TEST_CASE("[Modules][ECS] Test startup system order on removal.") {
	World world;
	world.add_databag<ExecutionCounter>();

	Pipeline pipeline;
	pipeline.add_startup_system(startup_system_2_test);
	pipeline.add_startup_system(startup_system_3_test);
	pipeline.add_startup_system(startup_system_4_test);
	pipeline.build();
	pipeline.prepare(&world);

	for (uint32_t i = 0; i < 10; i += 1) {
		pipeline.dispatch(&world);
	}

	const ExecutionCounter *counter = world.get_databag<ExecutionCounter>();
	// Make sure this run just twice.
	CHECK(counter->count == 5);
}
} // namespace godex_tests_startup_system

#endif // TEST_ECS_SYSTEM_H
