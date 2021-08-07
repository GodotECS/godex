#ifndef TEST_ECS_TEMPORARY_SYSTEM_H
#define TEST_ECS_TEMPORARY_SYSTEM_H

#include "tests/test_macros.h"

#include "../ecs.h"
#include "../pipeline/pipeline.h"
#include "../pipeline/pipeline_builder.h"
#include "../world/world.h"

struct ExecutionCounter : public godex::Databag {
	DATABAG(ExecutionCounter)

public:
	uint32_t prev = 0;
	uint32_t count = 0;
};

namespace godex_tests_temporary_system {

bool temporary_system_1_test(ExecutionCounter *p_counter) {
	p_counter->count += 1;
	return p_counter->count >= 2;
}

TEST_CASE("[Modules][ECS] Test temporary system.") {
	ECS::register_temporary_system(temporary_system_1_test, "temporary_system_1_test");

	ECS::register_databag<ExecutionCounter>();

	World world;
	world.create_databag<ExecutionCounter>();

	PipelineBuilder pipeline_builder;
	pipeline_builder.add_system("temporary_system_1_test");

	Pipeline pipeline;
	pipeline_builder.build(pipeline);
	Token token = pipeline.prepare_world(&world);
	pipeline.set_active(token, true);

	for (uint32_t i = 0; i < 10; i += 1) {
		pipeline.dispatch(token);
	}

	const ExecutionCounter *counter = world.get_databag<ExecutionCounter>();
	// Make sure this run just twice.
	CHECK(counter->count == 2);
}

TEST_CASE("[Modules][ECS] Test registered temporary system.") {
	ECS::register_temporary_system(temporary_system_1_test, "TemporarySystemTest");

	World world;
	world.create_databag<ExecutionCounter>();

	PipelineBuilder pipeline_builder;
	pipeline_builder.add_system("TemporarySystemTest");

	Pipeline pipeline;
	pipeline_builder.build(pipeline);
	Token token = pipeline.prepare_world(&world);
	pipeline.set_active(token, true);

	for (uint32_t i = 0; i < 10; i += 1) {
		pipeline.dispatch(token);
	}

	const ExecutionCounter *counter = world.get_databag<ExecutionCounter>();
	// Make sure this run just twice.
	CHECK(counter->count == 2);
}

bool temporary_system_2_test(ExecutionCounter *p_counter) {
	p_counter->count += 1;

	if (p_counter->count == 5) {
		return true;
	}

	return false;
}

bool temporary_system_3_test(ExecutionCounter *p_counter) {
	p_counter->count += 1;
	return true;
}

bool temporary_system_4_test(ExecutionCounter *p_counter) {
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

TEST_CASE("[Modules][ECS] Test temporary system order on removal.") {
	ECS::register_temporary_system(temporary_system_2_test, "temporary_system_2_test");
	ECS::register_temporary_system(temporary_system_3_test, "temporary_system_3_test");
	ECS::register_temporary_system(temporary_system_4_test, "temporary_system_4_test");

	World world;
	world.create_databag<ExecutionCounter>();

	PipelineBuilder pipeline_builder;
	pipeline_builder.add_system("temporary_system_2_test");
	pipeline_builder.add_system("temporary_system_3_test");
	pipeline_builder.add_system("temporary_system_4_test");

	Pipeline pipeline;
	pipeline_builder.build(pipeline);
	Token token = pipeline.prepare_world(&world);
	pipeline.set_active(token, true);

	for (uint32_t i = 0; i < 10; i += 1) {
		pipeline.dispatch(token);
	}

	const ExecutionCounter *counter = world.get_databag<ExecutionCounter>();
	// Make sure this run just twice.
	CHECK(counter->count == 5);
}
} // namespace godex_tests_temporary_system

#endif // TEST_ECS_SYSTEM_H
