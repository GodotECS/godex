#ifndef TEST_ECS_QUERY_H
#define TEST_ECS_QUERY_H

#include "tests/test_macros.h"

#include "../components/dynamic_component.h"
#include "../ecs.h"
#include "../godot/components/transform_component.h"
#include "../iterators/dynamic_query.h"
#include "../storage/batch_storage.h"
#include "../world/world.h"

class TagQueryTestComponent : public godex::Component {
	COMPONENT(TagQueryTestComponent, DenseVectorStorage)
};

class TestFixedSizeEvent : public godex::Component {
	COMPONENT_BATCH(TestFixedSizeEvent, DenseVector, 2)
public:
	int number = 0;

	TestFixedSizeEvent() {}
	TestFixedSizeEvent(int num) :
			number(num) {}
};

class TestEvent : public godex::Component {
	COMPONENT_BATCH(TestEvent, DenseVector, -1) // -1 make the storage dynamic.
public:
	int number = 0;

	TestEvent() {}
	TestEvent(int num) :
			number(num) {}
};

namespace godex_tests {

TEST_CASE("[Modules][ECS] Test static query") {
	ECS::register_component<TagQueryTestComponent>();

	World world;

	EntityID entity_1 = world
								.create_entity()
								.with(TransformComponent())
								.with(TagQueryTestComponent());

	EntityID entity_2 = world
								.create_entity()
								.with(TransformComponent(Transform(Basis(), Vector3(0.0, 0.0, 23.0))));

	EntityID entity_3 = world
								.create_entity()
								.with(TransformComponent())
								.with(TagQueryTestComponent());

	// Test `Without` filter.
	{
		Query<const TransformComponent, Without<TagQueryTestComponent>> query(&world);

		// This query fetches the entity that have only the `TransformComponent`.
		CHECK(query.is_done() == false);
		auto [transform, tag] = query.get();
		CHECK(ABS(transform->get_transform().origin.z - 23.0) <= CMP_EPSILON);
		CHECK(query.get_current_entity() == entity_2);
		CHECK(tag == nullptr);

		query.next();

		// Now it's done
		CHECK(query.is_done());
	}

	// Test `Maybe` filter.
	{
		Query<const TransformComponent, Maybe<TagQueryTestComponent>> query(&world);

		// This query fetches all entities but return nullptr when
		// `TagQueryTestComponent` is not set.
		{
			// Entity 1
			CHECK(query.is_done() == false);
			auto [transform, tag] = query.get();
			CHECK(query.get_current_entity() == entity_1);
			CHECK(transform != nullptr);
			CHECK(tag != nullptr);
		}

		query.next();

		{
			// Entity 2
			CHECK(query.is_done() == false);
			auto [transform, tag] = query.get();
			CHECK(query.get_current_entity() == entity_2);
			CHECK(transform != nullptr);
			CHECK(tag == nullptr);
		}

		query.next();

		{
			// Entity 3
			CHECK(query.is_done() == false);
			auto [transform, tag] = query.get();
			CHECK(query.get_current_entity() == entity_3);
			CHECK(transform != nullptr);
			CHECK(tag != nullptr);
		}

		query.next();

		// Now it's done
		CHECK(query.is_done());
	}
}

TEST_CASE("[Modules][ECS] Test static query check query type fetch.") {
	World world;

	{
		Query<const TransformComponent, Maybe<TagQueryTestComponent>> query(&world);

		LocalVector<uint32_t> mutable_components;
		LocalVector<uint32_t> immutable_components;
		query.get_components(mutable_components, immutable_components);

		CHECK(mutable_components.find(TagQueryTestComponent::get_component_id()) != -1);

		CHECK(immutable_components.find(TransformComponent::get_component_id()) != -1);
	}

	{
		Query<TransformComponent, Maybe<const TagQueryTestComponent>> query(&world);

		LocalVector<uint32_t> mutable_components;
		LocalVector<uint32_t> immutable_components;
		query.get_components(mutable_components, immutable_components);

		CHECK(mutable_components.find(TransformComponent::get_component_id()) != -1);

		CHECK(immutable_components.find(TagQueryTestComponent::get_component_id()) != -1);
	}

	{
		Query<Without<TransformComponent>, Maybe<const TagQueryTestComponent>> query(&world);

		LocalVector<uint32_t> mutable_components;
		LocalVector<uint32_t> immutable_components;
		query.get_components(mutable_components, immutable_components);

		CHECK(mutable_components.size() == 0);

		// `Without` filter collects the data always immutable.
		CHECK(immutable_components.find(TransformComponent::get_component_id()) != -1);
		CHECK(immutable_components.find(TagQueryTestComponent::get_component_id()) != -1);
	}
}

TEST_CASE("[Modules][ECS] Test dynamic query") {
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
		query.with_component(ECS::get_component_id("TransformComponent"), true);
		query.with_component(ECS::get_component_id("TagQueryTestComponent"), false);

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

		query.next();

		// Entity 3 (Entity 2 is skipped because it doesn't fulfil the query)
		CHECK(query.is_done() == false);
		CHECK(query.get_current_entity_id() == entity_3);

		{
			CHECK(query.access_count() == 2);
			CHECK(query.get_access(0)->is_mutable());
			CHECK(query.get_access(1)->is_mutable() == false);
			query.get_access(0)->set("transform", Transform(Basis(), Vector3(200.0, 200.0, 200.0)));
		}

		query.next();

		// Nothing more to do at this point.
		CHECK(query.is_done());
		query.end();
	}

	{
		godex::DynamicQuery query;
		query.with_component(ECS::get_component_id("TransformComponent"), false);

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

		query.next();

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

		query.next();

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

		query.next();

		CHECK(query.is_done());
		query.end();
	}

	{
		// Check the API `without_component()`.
		godex::DynamicQuery query;
		query.with_component(TransformComponent::get_component_id());
		query.without_component(TagQueryTestComponent::get_component_id());

		query.begin(&world);

		// This query fetches the entity that have only the `TransformComponent`.
		CHECK(query.is_done() == false);
		CHECK(query.get_current_entity_id() == entity_2);

		query.next();

		// Now it's done
		CHECK(query.is_done());

		query.end();
	}

	{
		// Check the API `maybe_component()`.
		godex::DynamicQuery query;
		query.with_component(TransformComponent::get_component_id());
		query.maybe_component(TagQueryTestComponent::get_component_id());
		query.build();

		query.begin(&world);

		// This query fetches the entity that have only the `TransformComponent`.
		CHECK(query.is_done() == false);
		CHECK(query.get_current_entity_id() == entity_1);
		CHECK(query.get_access(0)->__target != nullptr);
		CHECK(query.get_access(1)->__target != nullptr);
		query.next();

		CHECK(query.is_done() == false);
		CHECK(query.get_current_entity_id() == entity_2);
		CHECK(query.get_access(0)->__target != nullptr);
		CHECK(query.get_access(1)->__target == nullptr);
		query.next();

		CHECK(query.is_done() == false);
		CHECK(query.get_current_entity_id() == entity_3);
		CHECK(query.get_access(0)->__target != nullptr);
		CHECK(query.get_access(1)->__target != nullptr);
		query.next();

		// Now it's done
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
	query.with_component(test_dyn_component_id, true);

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

	query.next();

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

	query.next();
	CHECK(query.is_done());
	query.end();

	// ~~ Make sure the data got written using another immutable query. ~~

	query.reset();
	query.with_component(test_dyn_component_id, false);

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

	query.next();

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

	query.next();
	CHECK(query.is_done());
	query.end();
}

TEST_CASE("[Modules][ECS] Test invalid dynamic query.") {
	godex::DynamicQuery query;

	// Add an invalid component.
	query.with_component(ECS::get_component_id("ThisComponentDoesntExists"));

	CHECK(query.is_valid() == false);

	// Reset the query.
	query.reset();

	// Build it again but this time valid.
	query.with_component(ECS::get_component_id("TransformComponent"));
	CHECK(query.is_valid());
}

TEST_CASE("[Modules][ECS] Test query with event.") {
	ECS::register_component<TestEvent>();
	ECS::register_component<TestFixedSizeEvent>();

	World world;

	EntityID entity_1 = world
								.create_entity()
								.with(TransformComponent())
								.with(TestEvent(50))
								.with(TestEvent(38));

	EntityID entity_2 = world
								.create_entity()
								.with(TransformComponent())
								.with(TestFixedSizeEvent(645))
								.with(TestFixedSizeEvent(33))
								.with(TestFixedSizeEvent(78))
								.with(TestFixedSizeEvent(52));

	EntityID entity_3 = world
								.create_entity()
								.with(TransformComponent())
								.with(TestEvent());

	// Try the first query with dynamic sized batch storage.
	{
		Query<TransformComponent, TestEvent> query(&world);

		{
			CHECK(query.is_done() == false);

			auto [transform, event] = query.get();

			CHECK(query.get_current_entity() == entity_1);

			CHECK(transform.get_size() == 1);
			CHECK(transform != nullptr);

			CHECK(event.get_size() == 2);
			CHECK(event[0]->number == 50);
			CHECK(event[1]->number == 38);

			query.next();
		}

		{
			CHECK(query.is_done() == false);

			auto [transform, event] = query.get();

			CHECK(query.get_current_entity() == entity_3);

			CHECK(transform.get_size() == 1);
			CHECK(transform != nullptr);

			CHECK(event.get_size() == 1);
			CHECK(event[0]->number == 0);

			query.next();
		}

		// Now it's done!
		CHECK(query.is_done());
	}

	// Try the second query with fixed sized batch storage.
	{
		Query<TransformComponent, TestFixedSizeEvent> query(&world);

		{
			CHECK(query.is_done() == false);

			auto [transform, event] = query.get();

			CHECK(query.get_current_entity() == entity_2);

			CHECK(transform.get_size() == 1);
			CHECK(transform != nullptr);

			CHECK(event.get_size() == 2);
			CHECK(event[0]->number == 645);
			CHECK(event[1]->number == 33);

			query.next();
		}
	}
}
} // namespace godex_tests

#endif // TEST_ECS_QUERY_H
