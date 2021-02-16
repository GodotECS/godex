#ifndef TEST_ECS_QUERY_H
#define TEST_ECS_QUERY_H

#include "tests/test_macros.h"

#include "../components/dynamic_component.h"
#include "../ecs.h"
#include "../godot/components/transform_component.h"
#include "../iterators/dynamic_query.h"
#include "../storage/batch_storage.h"
#include "../world/world.h"

struct TagQueryTestComponent {
	COMPONENT(TagQueryTestComponent, DenseVectorStorage)
	static void _bind_methods() {}
};

struct TestFixedSizeEvent {
	COMPONENT_BATCH(TestFixedSizeEvent, DenseVector, 2)
	static void _bind_methods() {}

	int number = 0;

	TestFixedSizeEvent(int num) :
			number(num) {}
};

struct TestEvent {
	COMPONENT_BATCH(TestEvent, DenseVector, -1) // -1 make the storage dynamic.
	static void _bind_methods() {}

	int number = 0;

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
		CHECK(ABS(transform->transform.origin.z - 23.0) <= CMP_EPSILON);
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
} // namespace godex_tests

/// Used to trace the query mutability access the storage.
template <class T>
class AccessTracerStorage : public Storage<T> {
	DenseVector<T> storage;

public:
	uint32_t count_insert = 0;
	uint32_t count_has = 0;
	uint32_t count_remove = 0;
	uint32_t count_get_mut = 0;
	uint32_t count_get_immut = 0;
	uint32_t count_clear = 0;

	virtual String get_type_name() const override {
		return "AccessTranceStorage";
	}

	virtual void insert(EntityID p_entity, const T &p_data) override {
		count_insert += 1;
		storage.insert(p_entity, p_data);
		StorageBase::notify_changed(p_entity);
	}

	virtual bool has(EntityID p_entity) const override {
		const_cast<AccessTracerStorage<T> *>(this)->count_has += 1;
		return storage.has(p_entity);
	}

	virtual Batch<const std::remove_const_t<T>> get(EntityID p_entity, Space p_mode = Space::LOCAL) const override {
		const_cast<AccessTracerStorage<T> *>(this)->count_get_immut += 1;
		return &storage.get(p_entity);
	}

	virtual Batch<std::remove_const_t<T>> get(EntityID p_entity, Space p_mode = Space::LOCAL) override {
		StorageBase::notify_changed(p_entity);
		count_get_mut += 1;
		return &storage.get(p_entity);
	}

	virtual void remove(EntityID p_entity) override {
		StorageBase::notify_updated(p_entity);
		count_remove += 1;
		storage.remove(p_entity);
	}

	virtual void clear() override {
		count_clear += 1;
		storage.clear();
		StorageBase::flush_changed();
	}
};

struct TestAccessMutabilityComponent1 {
	COMPONENT(TestAccessMutabilityComponent1, AccessTracerStorage)
	static void _bind_methods() {}
};

struct TestAccessMutabilityComponent2 {
	COMPONENT(TestAccessMutabilityComponent2, AccessTracerStorage)
	static void _bind_methods() {}
};

namespace godex_tests {
TEST_CASE("[Modules][ECS] Test query mutability.") {
	ECS::register_component<TestAccessMutabilityComponent1>();
	ECS::register_component<TestAccessMutabilityComponent2>();

	World world;

	world
			.create_entity()
			.with(TestAccessMutabilityComponent1());

	world
			.create_entity()
			.with(TestAccessMutabilityComponent2());

	// Make sure the query access to the storage with the correct mutability.

	AccessTracerStorage<TestAccessMutabilityComponent1> *storage1 = static_cast<AccessTracerStorage<TestAccessMutabilityComponent1> *>(world.get_storage<TestAccessMutabilityComponent1>());
	AccessTracerStorage<TestAccessMutabilityComponent2> *storage2 = static_cast<AccessTracerStorage<TestAccessMutabilityComponent2> *>(world.get_storage<TestAccessMutabilityComponent2>());

	// Just two insert.
	CHECK(storage1->count_insert == 1);
	CHECK(storage2->count_insert == 1);

	// Test mutable
	CHECK(storage1->count_get_mut == 0);

	Query<TestAccessMutabilityComponent1> query_test_mut(&world);
	query_test_mut.get();

	CHECK(storage1->count_get_mut == 1);
	CHECK(storage2->count_get_mut == 0);

	Query<Without<TestAccessMutabilityComponent1>, TestAccessMutabilityComponent2> query_test_without_mut(&world);
	query_test_without_mut.get();

	CHECK(storage1->count_get_mut == 1);
	CHECK(storage2->count_get_mut == 1);

	Query<Maybe<TestAccessMutabilityComponent1>> query_test_maybe_mut(&world);
	query_test_maybe_mut.get();

	CHECK(storage1->count_get_mut == 2);

	// Test immutables

	CHECK(storage1->count_get_immut == 0);

	Query<const TestAccessMutabilityComponent1> query_test_immut(&world);
	query_test_immut.get();

	CHECK(storage1->count_get_mut == 2);
	CHECK(storage1->count_get_immut == 1);
	CHECK(storage2->count_get_immut == 0);

	Query<Without<const TestAccessMutabilityComponent1>, const TestAccessMutabilityComponent2> query_test_without_immut(&world);
	query_test_without_immut.get();

	CHECK(storage1->count_get_mut == 2);
	CHECK(storage1->count_get_immut == 1);
	CHECK(storage2->count_get_immut == 1);

	Query<Maybe<const TestAccessMutabilityComponent1>> query_test_maybe_immut(&world);
	query_test_maybe_immut.get();

	CHECK(storage1->count_get_mut == 2);
	CHECK(storage1->count_get_immut == 2);
}

TEST_CASE("[Modules][ECS] Test DynamicQuery fetch mutability.") {
	World world;

	world
			.create_entity()
			.with(TestAccessMutabilityComponent1());

	world
			.create_entity()
			.with(TestAccessMutabilityComponent2());

	// Make sure the query access to the storage with the correct mutability.

	AccessTracerStorage<TestAccessMutabilityComponent1> *storage1 = static_cast<AccessTracerStorage<TestAccessMutabilityComponent1> *>(world.get_storage<TestAccessMutabilityComponent1>());
	AccessTracerStorage<TestAccessMutabilityComponent2> *storage2 = static_cast<AccessTracerStorage<TestAccessMutabilityComponent2> *>(world.get_storage<TestAccessMutabilityComponent2>());

	// Just two insert.
	CHECK(storage1->count_insert == 1);
	CHECK(storage2->count_insert == 1);

	// Test mutable
	CHECK(storage1->count_get_mut == 0);

	godex::DynamicQuery query_test_mut;
	query_test_mut.with_component(TestAccessMutabilityComponent1::get_component_id(), true);
	query_test_mut.begin(&world);

	CHECK(storage1->count_get_mut == 1);
	CHECK(storage2->count_get_mut == 0);

	godex::DynamicQuery query_test_without_mut;
	query_test_without_mut.without_component(TestAccessMutabilityComponent1::get_component_id());
	query_test_without_mut.with_component(TestAccessMutabilityComponent2::get_component_id(), true);
	query_test_without_mut.begin(&world);

	CHECK(storage1->count_get_mut == 1);
	CHECK(storage2->count_get_mut == 1);

	// Make sure `maybe` alone doesn't fetch anything.
	godex::DynamicQuery query_test_maybe_alone_mut;
	query_test_maybe_alone_mut.maybe_component(TestAccessMutabilityComponent1::get_component_id(), true);
	query_test_maybe_alone_mut.begin(&world);

	CHECK(storage1->count_get_mut == 1);

	// Test immutables

	CHECK(storage1->count_get_immut == 0);

	godex::DynamicQuery query_test_immut;
	query_test_immut.with_component(TestAccessMutabilityComponent1::get_component_id(), false);
	query_test_immut.begin(&world);
	CHECK(storage1->count_get_mut == 1);
	CHECK(storage1->count_get_immut == 1);

	CHECK(storage2->count_get_immut == 0);
	godex::DynamicQuery query_test_without_immut;
	query_test_without_immut.without_component(TestAccessMutabilityComponent1::get_component_id());
	query_test_without_immut.with_component(TestAccessMutabilityComponent2::get_component_id(), false);
	query_test_without_immut.begin(&world);

	CHECK(storage1->count_get_mut == 1);
	CHECK(storage1->count_get_immut == 1);
	CHECK(storage2->count_get_immut == 1);

	godex::DynamicQuery query_test_maybe_alone_immut;
	query_test_maybe_alone_immut.maybe_component(TestAccessMutabilityComponent1::get_component_id(), false);
	query_test_maybe_alone_immut.begin(&world);

	CHECK(storage1->count_get_mut == 1);
	CHECK(storage1->count_get_immut == 1);
}

TEST_CASE("[Modules][ECS] Test DynamicQuery try fetch combination.") {
	World world;

	EntityID entity_1 = world
								.create_entity()
								.with(TestAccessMutabilityComponent1());

	world
			.create_entity()
			.with(TestAccessMutabilityComponent2());

	EntityID entity_3 = world
								.create_entity()
								.with(TestAccessMutabilityComponent1())
								.with(TestAccessMutabilityComponent2());

	{
		// Check `With`  and `With`.
		godex::DynamicQuery query;
		query.with_component(TestAccessMutabilityComponent1::get_component_id(), false);
		query.with_component(TestAccessMutabilityComponent2::get_component_id(), false);
		query.begin(&world);
		CHECK(query.get_current_entity_id() == entity_3);
	}

	{
		// Check `With`  and `Without`.
		godex::DynamicQuery query;
		query.with_component(TestAccessMutabilityComponent1::get_component_id(), false);
		query.without_component(TestAccessMutabilityComponent2::get_component_id());
		query.begin(&world);
		CHECK(query.get_current_entity_id() == entity_1);
	}

	{
		// Check `With`  and `Maybe`.
		godex::DynamicQuery query;
		query.with_component(TestAccessMutabilityComponent1::get_component_id(), false);
		query.maybe_component(TestAccessMutabilityComponent2::get_component_id());
		query.begin(&world);
		CHECK(query.get_current_entity_id() == entity_1);
		query.next();
		CHECK(query.get_current_entity_id() == entity_3);
	}

	{
		// Check `Maybe`.
		godex::DynamicQuery query;
		query.maybe_component(TestAccessMutabilityComponent2::get_component_id());
		query.begin(&world);
		// Nothing to fetch, because `Maybe` alone is meaningless.
		CHECK(query.is_done());
	}

	{
		// Check `Without`.
		godex::DynamicQuery query;
		query.without_component(TestAccessMutabilityComponent2::get_component_id());
		query.begin(&world);
		// Nothing to fetch, because `Without` alone is meaningless.
		CHECK(query.is_done());
	}

	{
		// Check `Maybe` and `Without`.
		godex::DynamicQuery query;
		query.maybe_component(TestAccessMutabilityComponent1::get_component_id());
		query.without_component(TestAccessMutabilityComponent2::get_component_id());
		query.begin(&world);
		// Nothing to fetch, because `Maybe` and `Without` are meaningless alone.
		CHECK(query.is_done());
	}
}

TEST_CASE("[Modules][ECS] Test DynamicQuery changed.") {
	World world;

	EntityID entity_1 = world
								.create_entity()
								.with(TestAccessMutabilityComponent1());

	world.get_storage(TestAccessMutabilityComponent1::get_component_id())->set_need_changed(true);

	{
		// Trigger changed.
		godex::DynamicQuery query;
		query.with_component(TestAccessMutabilityComponent1::get_component_id(), true);
		query.begin(&world);
		CHECK(query.get_current_entity_id() == entity_1);
	}

	{
		// Check I can access changed.
		godex::DynamicQuery query;
		query.changed_component(TestAccessMutabilityComponent1::get_component_id(), false);
		query.begin(&world);
		CHECK(query.get_current_entity_id() == entity_1);
	}

	world.get_storage(TestAccessMutabilityComponent1::get_component_id())->flush_changed();

	{
		// Take the component again, without triggering changed.
		godex::DynamicQuery query;
		query.with_component(TestAccessMutabilityComponent1::get_component_id(), false);
		query.begin(&world);
		CHECK(query.get_current_entity_id() == entity_1);
	}

	{
		// Check changed is gone.
		godex::DynamicQuery query;
		query.changed_component(TestAccessMutabilityComponent1::get_component_id(), false);
		query.begin(&world);
		CHECK(query.is_done());
	}
}

TEST_CASE("[Modules][ECS] Test static query check query type fetch.") {
	World world;

	{
		Query<const TransformComponent, Maybe<TagQueryTestComponent>> query(&world);

		SystemExeInfo info;
		query.get_components(info);

		CHECK(info.mutable_components.find(TagQueryTestComponent::get_component_id()) != nullptr);
		CHECK(info.immutable_components.find(TransformComponent::get_component_id()) != nullptr);
	}

	{
		Query<TransformComponent, Maybe<const TagQueryTestComponent>> query(&world);

		SystemExeInfo info;
		query.get_components(info);

		CHECK(info.mutable_components.find(TransformComponent::get_component_id()) != nullptr);

		CHECK(info.immutable_components.find(TagQueryTestComponent::get_component_id()) != nullptr);
	}

	{
		Query<Without<TransformComponent>, Maybe<const TagQueryTestComponent>> query(&world);

		SystemExeInfo info;
		query.get_components(info);

		CHECK(info.mutable_components.size() == 0);

		// `Without` filter collects the data always immutable.
		CHECK(info.immutable_components.find(TransformComponent::get_component_id()) != nullptr);
		CHECK(info.immutable_components.find(TagQueryTestComponent::get_component_id()) != nullptr);
	}

	{
		Query<Changed<TransformComponent>, Changed<const TagQueryTestComponent>> query(&world);

		SystemExeInfo info;
		query.get_components(info);

		CHECK(info.mutable_components.find(TransformComponent::get_component_id()) != nullptr);
		CHECK(info.immutable_components.find(TagQueryTestComponent::get_component_id()) != nullptr);
		CHECK(info.need_changed.find(TransformComponent::get_component_id()) != nullptr);
		CHECK(info.need_changed.find(TagQueryTestComponent::get_component_id()) != nullptr);
	}
}

TEST_CASE("[Modules][ECS] Test static query filter no storage.") {
	World world;

	{
		Query<Without<TagQueryTestComponent>, TransformComponent> query(&world);

		// No storage, make sure this returns immediately.
		CHECK(query.is_done());
	}

	{
		Query<Maybe<TagQueryTestComponent>, TransformComponent> query(&world);

		// No storage, make sure this returns immediately.
		CHECK(query.is_done());
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
		CHECK(query.get_access(0)->get_target() != nullptr);
		CHECK(query.get_access(1)->get_target() != nullptr);
		query.next();

		CHECK(query.is_done() == false);
		CHECK(query.get_current_entity_id() == entity_2);
		CHECK(query.get_access(0)->get_target() != nullptr);
		CHECK(query.get_access(1)->get_target() == nullptr);
		query.next();

		CHECK(query.is_done() == false);
		CHECK(query.get_current_entity_id() == entity_3);
		CHECK(query.get_access(0)->get_target() != nullptr);
		CHECK(query.get_access(1)->get_target() != nullptr);
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

	EntityID entity_1 = world
								.create_entity()
								.with(test_dyn_component_id, Dictionary());

	EntityID entity_2 = world
								.create_entity()
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

TEST_CASE("[Modules][ECS] Test query random Entity access.") {
	World world;

	EntityID entity_1 = world
								.create_entity()
								.with(TransformComponent())
								.with(TestEvent(50));

	EntityID entity_2 = world
								.create_entity()
								.with(TransformComponent());

	EntityID entity_3 = world
								.create_entity()
								.with(TransformComponent())
								.with(TestEvent());

	Query<TransformComponent, TestEvent> query(&world);

	CHECK(query.fetch_entity(entity_1));
	CHECK(query.get_current_entity() == entity_1);
	{
		auto [transform, event] = query.get();
		CHECK(event->number == 50);
	}

	CHECK(query.fetch_entity(entity_2) == false);

	CHECK(query.fetch_entity(entity_3));
	CHECK(query.get_current_entity() == entity_3);
	{
		auto [transform, event] = query.get();
		CHECK(event->number == 0);
	}
}
} // namespace godex_tests

#endif // TEST_ECS_QUERY_H
