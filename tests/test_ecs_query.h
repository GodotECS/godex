#ifndef TEST_ECS_QUERY_H
#define TEST_ECS_QUERY_H

#include "tests/test_macros.h"

#include "modules/ecs/components/dynamic_component.h"
#include "modules/ecs/components/transform_component.h"
#include "modules/ecs/ecs.h"
#include "modules/ecs/iterators/dynamic_query.h"
#include "modules/ecs/world/world.h"

class TagQueryTestComponent : public godex::Component {
	COMPONENT(TagQueryTestComponent, DenseVector)
};

namespace godex_tests {

TEST_CASE("[Modules][ECS] Test dynamic query") {
	ECS::register_component<TagQueryTestComponent>();

	World world;

	EntityID entity_1 = world
								.create_entity()
								.with(TransformComponent())
								.with(TagQueryTestComponent());

	EntityID entity_2 = world
								.create_entity()
								.with(TransformComponent());

	EntityID entity_3 = world
								.create_entity()
								.with(TransformComponent())
								.with(TagQueryTestComponent());

	{
		godex::DynamicQuery query;
		query.add_component(ECS::get_component_id("TransformComponent"), true);
		query.add_component(ECS::get_component_id("TagQueryTestComponent"), false);

		CHECK(query.is_valid());

		// Test process.
		query.begin(&world);

		// Entity 1
		CHECK(query.is_done() == false);
		CHECK(query.get_current_entity_id() == entity_1);

		{
			CHECK(query.access_count() == 2);
			CHECK(query.get_access(0)->is_mutable());
			CHECK(query.get_access(1)->is_mutable() == false);
			query.get_access(0)->set("transform", Transform(Basis(), Vector3(100.0, 100.0, 100.0)));
		}

		query.next_entity();

		// Entity 3 (Entity 2 is skipped because it doesn't fulfil the query)
		CHECK(query.is_done() == false);
		CHECK(query.get_current_entity_id() == entity_3);

		{
			CHECK(query.access_count() == 2);
			CHECK(query.get_access(0)->is_mutable());
			CHECK(query.get_access(1)->is_mutable() == false);
			query.get_access(0)->set("transform", Transform(Basis(), Vector3(200.0, 200.0, 200.0)));
		}

		query.next_entity();

		// Nothing more to do at this point.
		CHECK(query.is_done());
		query.end();
	}

	{
		godex::DynamicQuery query;
		query.add_component(ECS::get_component_id("TransformComponent"), false);

		// Test process.
		query.begin(&world);

		// Entity 1
		CHECK(query.is_done() == false);
		CHECK(query.get_current_entity_id() == entity_1);

		{
			CHECK(query.access_count() == 1);
			CHECK(query.get_access(0)->is_mutable() == false);
			const Transform t = query.get_access(0)->get("transform");
			// Check if the entity_1 is changed.
			CHECK(t.origin.x - 100.0 <= CMP_EPSILON);
		}

		query.next_entity();

		// Entity 2
		CHECK(query.is_done() == false);
		CHECK(query.get_current_entity_id() == entity_2);
		{
			CHECK(query.access_count() == 1);
			CHECK(query.get_access(0)->is_mutable() == false);
			const Transform t = query.get_access(0)->get("transform");
			// Make sure the entity_2 is not changed.
			CHECK(t.origin.x <= CMP_EPSILON);
		}

		query.next_entity();

		// Entity 3
		CHECK(query.is_done() == false);
		CHECK(query.get_current_entity_id() == entity_3);
		{
			CHECK(query.access_count() == 1);
			CHECK(query.get_access(0)->is_mutable() == false);
			const Transform t = query.get_access(0)->get("transform");
			// Make sure the entity_3 is changed.
			CHECK(t.origin.x - 200.0 <= CMP_EPSILON);
		}

		query.next_entity();

		CHECK(query.is_done());
		query.end();
	}
}

TEST_CASE("[Modules][ECS] Test dynamic query with dynamic storages.") {
	LocalVector<ScriptProperty> props;
	props.push_back({ PropertyInfo(Variant::INT, "variable_1"), 1 });
	props.push_back({ PropertyInfo(Variant::BOOL, "variable_2"), false });
	props.push_back({ PropertyInfo(Variant::VECTOR3, "variable_3"), Vector3() });

	const uint32_t test_dyn_component_id = ECS::register_script_component(
			"TestDynamicQueryComponent1.gd",
			props,
			StorageType::DENSE_VECTOR);

	World world;

	EntityID entity_1 = world.create_entity()
								.with(test_dyn_component_id, Dictionary());

	EntityID entity_2 = world.create_entity()
								.with(test_dyn_component_id, Dictionary());

	godex::DynamicQuery query;
	query.add_component(test_dyn_component_id, true);

	CHECK(query.is_valid());

	// Test process.
	query.begin(&world);

	// Entity 1
	CHECK(query.is_done() == false);
	CHECK(query.get_current_entity_id() == entity_1);

	{
		CHECK(query.access_count() == 1);
		CHECK(query.get_access(0)->is_mutable());
		query.get_access(0)->set("variable_1", 20);
		query.get_access(0)->set("variable_2", true);
		query.get_access(0)->set("variable_3", Vector2(10., 10.)); // Test if wrong type is still handled: `variable_3` is a `Vector3`.
	}

	query.next_entity();

	// Entity 2
	CHECK(query.is_done() == false);
	CHECK(query.get_current_entity_id() == entity_2);

	{
		CHECK(query.access_count() == 1);
		CHECK(query.get_access(0)->is_mutable());
		query.get_access(0)->set("variable_1", 30);
		query.get_access(0)->set("variable_2", true);
		query.get_access(0)->set("variable_3", Vector3(10.0, 0, 0));
	}

	query.next_entity();
	CHECK(query.is_done());
	query.end();

	// ~~ Make sure the data got written using another immutable query. ~~

	query.reset();
	query.add_component(test_dyn_component_id, false);

	query.begin(&world);

	// Entity 1
	CHECK(query.is_done() == false);
	CHECK(query.get_current_entity_id() == entity_1);

	{
		CHECK(query.access_count() == 1);
		CHECK(query.get_access(0)->is_mutable() == false);
		CHECK(query.get_access(0)->get("variable_1") == Variant(20));
		CHECK(query.get_access(0)->get("variable_2") == Variant(true));
		// Make sure this doesn't changed, since we submitted a wrong value.
		CHECK(query.get_access(0)->get("variable_3") == Variant(Vector3()));
	}

	query.next_entity();

	// Entity 2
	CHECK(query.is_done() == false);
	CHECK(query.get_current_entity_id() == entity_2);

	{
		CHECK(query.access_count() == 1);
		CHECK(query.get_access(0)->is_mutable() == false);
		CHECK(query.get_access(0)->get("variable_1") == Variant(30));
		CHECK(query.get_access(0)->get("variable_2") == Variant(true));
		CHECK(ABS(query.get_access(0)->get("variable_3").operator Vector3().x - 10.0) < CMP_EPSILON);

		// Try to mutate an immutable value.
		query.get_access(0)->set("variable_2", false);
		CHECK(query.get_access(0)->get("variable_2") != Variant(false));
	}

	query.next_entity();
	CHECK(query.is_done());
	query.end();
}

TEST_CASE("[Modules][ECS] Test invalid dynamic query.") {
	godex::DynamicQuery query;

	// Add an invalid component.
	query.add_component(ECS::get_component_id("ThisComponentDoesntExists"));

	CHECK(query.is_valid() == false);

	// Reset the query.
	query.reset();

	// Build it again but this time valid.
	query.add_component(ECS::get_component_id("TransformComponent"));
	CHECK(query.is_valid());
}

} // namespace godex_tests

#endif // TEST_ECS_QUERY_H
