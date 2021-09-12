#ifndef TEST_ECS_PIPELINE_H
#define TEST_ECS_PIPELINE_H

#include "tests/test_macros.h"

#include "../ecs.h"
#include "../modules/godot/components/transform_component.h"
#include "../pipeline/pipeline.h"
#include "../pipeline/pipeline_builder.h"
#include "../pipeline/pipeline_commands.h"
#include "../storage/dense_vector_storage.h"
#include "../systems/dynamic_system.h"

class PipelineTestDatabag1 : public godex::Databag {
	DATABAG(PipelineTestDatabag1)

public:
	int a = 10;
};

namespace godex_tests_pipeline {

void system_with_databag(PipelineTestDatabag1 *test_res) {}

TEST_CASE("[Modules][ECS] Test pipeline build.") {
	godex::system_id system_id = ECS::register_system(system_with_databag, "system_with_databag").get_id();
	ECS::register_databag<PipelineTestDatabag1>();

	PipelineBuilder pipeline_builder;
	pipeline_builder.add_system(system_id);

	Pipeline pipeline;

	// Make sure the pipeline is not yet ready.
	CHECK(pipeline.is_ready() == false);

	pipeline_builder.build(pipeline);

	// Make sure the pipeline is now ready.
	CHECK(pipeline.is_ready());

	pipeline.reset();

	// Make sure the pipeline is not ready again.
	CHECK(pipeline.is_ready() == false);
}
} // namespace godex_tests_pipeline

namespace godex_tests_pipeline {

void test_trigger_changed_1(PipelineCommands *p_pipeline_command, Query<TransformComponent> &p_query) {
	p_pipeline_command->set_active_system(SNAME("test_get_changed"), false);
	{
		auto [transform] = p_query[0];
		transform->origin.x += 100.0;
	}
	{
		auto [transform] = p_query[1];
		transform->origin.x += 100.0;
	}
	p_pipeline_command->set_active_system(SNAME("test_get_changed"), true);
}

void test_trigger_changed_2(Query<TransformComponent> &p_query) {
	{
		auto [transform] = p_query[2];
		transform->origin.x += 200.0;
	}
}

void test_get_changed(Query<Changed<TransformComponent>> &p_query) {
	CHECK(!p_query.has(0));
	CHECK(!p_query.has(1));
	CHECK(p_query.has(2));
}

TEST_CASE("[Modules][ECS] Test `PipelineCommands`.") {
	// Creates a pipeline with a system that fetches mutably the entity 0 and 1,
	// but disables the `test_get_changed`: so it doesn't receives the changed
	// notifications.
	// Another system that mutates the entity 2.
	// The last system, make sure only the entity 2 is taken as changed.
	Pipeline pipeline;

	{
		godex::system_id system_1_id = ECS::register_system(test_trigger_changed_1, "test_trigger_changed_1").get_id();
		godex::system_id system_2_id = ECS::register_system(test_trigger_changed_2, "test_trigger_changed_2").get_id();
		godex::system_id system_3_id = ECS::register_system(test_get_changed, "test_get_changed").get_id();

		PipelineBuilder pipeline_builder;
		pipeline_builder.add_system(system_1_id);
		pipeline_builder.add_system(system_2_id);
		pipeline_builder.add_system(system_3_id);

		pipeline_builder.build(pipeline);
	}

	World world;
	const EntityID entity_1 = world
									  .create_entity()
									  .with(TransformComponent());

	const EntityID entity_2 = world
									  .create_entity()
									  .with(TransformComponent());

	const EntityID entity_3 = world
									  .create_entity()
									  .with(TransformComponent());

	const Token token = pipeline.prepare_world(&world);
	pipeline.set_active(token, true);
	for (uint32_t i = 0; i < 3; i += 1) {
		pipeline.dispatch(token);
	}

	// Make sure we dispatcher 3 times
	Storage<TransformComponent> *storage = world.get_storage<TransformComponent>();
	CHECK(Math::is_equal_approx(storage->get(entity_1)->origin.x, real_t(300.0)));
	CHECK(Math::is_equal_approx(storage->get(entity_2)->origin.x, real_t(300.0)));
	CHECK(Math::is_equal_approx(storage->get(entity_3)->origin.x, real_t(600.0)));
}
} // namespace godex_tests_pipeline
#endif // TEST_ECS_PIPELINE_H
