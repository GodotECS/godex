#ifndef TEST_ECS_SPAWNER_H
#define TEST_ECS_SPAWNER_H

#include "tests/test_macros.h"

#include "../components/component.h"
#include "../ecs.h"
#include "../modules/godot/components/transform_component.h"
#include "../modules/godot/nodes/script_ecs.h"
#include "../pipeline/pipeline.h"
#include "../pipeline/pipeline_builder.h"
#include "../spawners/spawner.h"
#include "../storage/dense_vector_storage.h"
#include "test_utilities.h"

struct TestEventSpawner {
	SPAWNER(TestEventSpawner)
};

struct SpawnTestComponent {
	COMPONENT(SpawnTestComponent, DenseVectorStorage)
	SPAWNERS(TestEventSpawner)

	int a = 0;
};

namespace godex_tests {

TEST_CASE("[Modules][ECS] Test spawner C++ registration.") {
	ECS::register_spawner<TestEventSpawner>();

	{
		const LocalVector<godex::component_id> &spawnable = ECS::get_spawnable_components(TestEventSpawner::get_spawner_id());
		CHECK(spawnable.size() == 0);
	}

	ECS::register_component<SpawnTestComponent>();

	{
		const LocalVector<godex::spawner_id> &spawners = ECS::get_spawners(SpawnTestComponent::get_component_id());
		CHECK(spawners.size() == 1);
		CHECK(spawners[0] == TestEventSpawner::get_spawner_id());
	}

	{
		const LocalVector<godex::component_id> &spawnable = ECS::get_spawnable_components(TestEventSpawner::get_spawner_id());
		CHECK(spawnable.size() == 1);
		CHECK(spawnable[0] == SpawnTestComponent::get_component_id());
	}

	// Check the spawner is fetching component storage type correctly.
	{
		SystemExeInfo info;
		Spawner<TestEventSpawner>::get_components(info);
		CHECK(info.mutable_components_storage.size() == 1);
		CHECK(info.mutable_components_storage.has(SpawnTestComponent::get_component_id()));
	}
}
} // namespace godex_tests

void test_spawner_system(Spawner<TestEventSpawner> &p_test_event_spawner) {
	// Add 1
	SpawnTestComponent data;
	data.a = 300;
	p_test_event_spawner.insert(0, data);

	// Add 2
	p_test_event_spawner.insert_dynamic(ECS::get_component_id("TestSpawnScriptComponent.gd"), 1, Dictionary());

	// Add 3
	p_test_event_spawner.insert(2, TransformComponent());
}

namespace godex_tests {
TEST_CASE("[Modules][ECS] Test spawner Script registration.") {
	initialize_script_ecs();

	StringName component_name = "TestSpawnScriptComponent.gd";

	{
		// Create the script.
		String code;
		code += "extends Component\n";
		code += "\n";
		code += "func get_spawners():\n";
		code += "	return ['TestEventSpawner']\n";
		code += "\n";
		code += "var aa: int = 0\n";
		code += "var bb: int = 0\n";
		code += "\n";

		CHECK(register_ecs_script(component_name, code));
	}

	build_scripts();
	flush_ecs_script_preparation();

	// Compose the script component
	Ref<Component> component = ScriptEcs::get_singleton()->get_script_component(component_name);
	CHECK(component.is_valid());

	// Make sure the spawners extractions is working
	{
		Vector<StringName> spawners = component->get_spawners();
		CHECK(spawners.size() == 1);
		CHECK(spawners[0] == StringName("TestEventSpawner"));
	}

	// Make sure the spawners are correctly set during component initialization.
	{
		const LocalVector<godex::spawner_id> &spawners = ECS::get_spawners(ECS::get_component_id(component->get_name()));
		CHECK(spawners.size() == 1);
		CHECK(spawners[0] == TestEventSpawner::get_spawner_id());

		const LocalVector<godex::component_id> &spawnable = ECS::get_spawnable_components(TestEventSpawner::get_spawner_id());
		CHECK(spawnable.size() == 2);
		CHECK(spawnable.find(SpawnTestComponent::get_component_id()) != -1);
		CHECK(spawnable.find(ECS::get_component_id(component->get_name())) != -1);
	}

	// Check the spawner is fetching component storage type correctly.
	{
		SystemExeInfo info;
		Spawner<TestEventSpawner>::get_components(info);
		CHECK(info.mutable_components_storage.size() == 2);
		CHECK(info.mutable_components_storage.has(SpawnTestComponent::get_component_id()));
		CHECK(info.mutable_components_storage.has(ECS::get_component_id(component->get_name())));
	}

	const godex::system_id system_id = ECS::register_system(test_spawner_system, "test_spawner_system").get_id();

	// Try to use the spawner now.
	World world;

	PipelineBuilder pipeline_builder;
	pipeline_builder.add_system(system_id);

	Pipeline pipeline;
	pipeline_builder.build(pipeline);
	Token token = pipeline.prepare_world(&world);
	pipeline.set_active(token, true);

	// Dispatch 1 time the pipeline.
	pipeline.dispatch(token);

	// Make sure the MySpawnableComponent is added
	{
		const Storage<const SpawnTestComponent> *storage = world.get_storage<const SpawnTestComponent>();
		CHECK(storage->has(0));
		CHECK(storage->get(0)->a == 300);
	}

	// Make sure the `TestSpawnScriptComponent.gd` is added.
	{
		const StorageBase *storage = world.get_storage(ECS::get_component_id(component_name));
		CHECK(storage->has(1));
	}

	// Make sure the TransformComponent is NOT added, since it's not a component
	// spawnable by `TestEventSpawner`.
	{
		const Storage<const TransformComponent> *storage = world.get_storage<const TransformComponent>();
		CHECK(storage == nullptr);
	}
	finalize_script_ecs();
}
} // namespace godex_tests

#endif // TEST_ECS_SPAWNER_H
