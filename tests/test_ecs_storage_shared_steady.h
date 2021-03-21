#ifndef TEST_SHARED_STEADY_STORAGE_H
#define TEST_SHARED_STEADY_STORAGE_H

#include "../components/component.h"
#include "../ecs.h"
#include "../storage/shared_steady_storage.h"
#include "core/math/math_funcs.h"

#include "tests/test_macros.h"

namespace godex_ecs_shared_steady_storage_tests {

struct SharedSteadyComponentTest {
	COMPONENT_CUSTOM_STORAGE(SharedSteadyComponentTest)

	int number = 0;

	SharedSteadyComponentTest(int i) :
			number(i) {}
};

TEST_CASE("[SharedSteadyStorage] Check memory share.") {
	SharedSteadyStorage<SharedSteadyComponentTest> storage;
	Dictionary config;
	config["page_size"] = 5;
	storage.configure(config);

	const godex::SID shared_component_id_1 = storage.create_shared_component(
			SharedSteadyComponentTest(10));
	{
		storage.insert(0, shared_component_id_1);
		storage.insert(1, shared_component_id_1);
		storage.insert(2, shared_component_id_1);
		storage.insert(3, shared_component_id_1);

		const SharedSteadyComponentTest *comp_e_0 = storage.get(0);
		const SharedSteadyComponentTest *comp_e_1 = storage.get(1);
		const SharedSteadyComponentTest *comp_e_2 = storage.get(2);
		const SharedSteadyComponentTest *comp_e_3 = storage.get(3);

		// Make sure all IDs are the same.
		CHECK(comp_e_0 == comp_e_1);
		CHECK(comp_e_0 == comp_e_2);
		CHECK(comp_e_0 == comp_e_3);
	}

	{
		SharedSteadyComponentTest *comp_e_0 = storage.get(0);
		CHECK(comp_e_0->number == 10);
		comp_e_0->number = 33;

		const SharedSteadyComponentTest *comp_e_1 = storage.get(1);
		CHECK(comp_e_1->number == 33);
	}

	const godex::SID shared_component_id_2 = storage.create_shared_component(
			SharedSteadyComponentTest(55));

	{
		storage.insert(1, shared_component_id_2);

		const SharedSteadyComponentTest *comp_e_0 = storage.get(0);
		const SharedSteadyComponentTest *comp_e_1 = storage.get(1);
		const SharedSteadyComponentTest *comp_e_2 = storage.get(2);
		const SharedSteadyComponentTest *comp_e_3 = storage.get(3);

		// Make These two IDs are now different.
		CHECK(comp_e_0 != comp_e_1);
		// But thow three are still the same.
		CHECK(comp_e_0 == comp_e_2);
		CHECK(comp_e_0 == comp_e_3);
	}

	{
		// Change the component data and later assign it to a new entity.
		{
			SharedSteadyComponentTest *comp_e_1 = storage.get(1);
			comp_e_1->number = 99;
		}

		storage.insert(3, shared_component_id_2);

		// Validate data.
		const SharedSteadyComponentTest *comp_e_0 = storage.get(0);
		const SharedSteadyComponentTest *comp_e_1 = storage.get(1);
		const SharedSteadyComponentTest *comp_e_2 = storage.get(2);
		const SharedSteadyComponentTest *comp_e_3 = storage.get(3);
		CHECK(comp_e_0 == comp_e_2);
		CHECK(comp_e_0->number == 33);

		CHECK(comp_e_1 == comp_e_3);
		CHECK(comp_e_1->number == 99);
	}

	// Free the shared component.
	{
		storage.free_shared_component(shared_component_id_1);

		// Make sure the Entity doesn't have that storage anymore.
		CHECK(storage.has(0) == false);
		CHECK(storage.has(2) == false);

		// Make sure the other still have the shared component.
		CHECK(storage.get(1)->number == 99);
		CHECK(storage.get(3)->number == 99);
	}

	// Remove the component from the entity and check if it still exists.
	{
		storage.remove(1);
		storage.remove(3);
		CHECK(storage.has_shared_component(shared_component_id_2));
		CHECK(storage.get_shared_component(shared_component_id_2)->number == 99);
	}
}

TEST_CASE("[SharedSteadyStorage] Check memory steadness.") {
	SharedSteadyStorage<SharedSteadyComponentTest> storage;
	Dictionary config;
	config["page_size"] = 5;
	storage.configure(config);

	const godex::SID shared_component_id_1 = storage.create_shared_component(
			SharedSteadyComponentTest(10));
	const SharedSteadyComponentTest *shared_component_id_1_ptr = storage.get_shared_component(shared_component_id_1);

	const godex::SID shared_component_id_2 = storage.create_shared_component(
			SharedSteadyComponentTest(10));
	const SharedSteadyComponentTest *shared_component_id_2_ptr = storage.get_shared_component(shared_component_id_2);

	const godex::SID shared_component_id_3 = storage.create_shared_component(
			SharedSteadyComponentTest(10));
	const SharedSteadyComponentTest *shared_component_id_3_ptr = storage.get_shared_component(shared_component_id_3);

	const godex::SID shared_component_id_4 = storage.create_shared_component(
			SharedSteadyComponentTest(10));
	const SharedSteadyComponentTest *shared_component_id_4_ptr = storage.get_shared_component(shared_component_id_4);

	const godex::SID shared_component_id_5 = storage.create_shared_component(
			SharedSteadyComponentTest(10));
	const SharedSteadyComponentTest *shared_component_id_5_ptr = storage.get_shared_component(shared_component_id_5);

	const godex::SID shared_component_id_6 = storage.create_shared_component(
			SharedSteadyComponentTest(10));
	const SharedSteadyComponentTest *shared_component_id_6_ptr = storage.get_shared_component(shared_component_id_6);

	const godex::SID shared_component_id_7 = storage.create_shared_component(
			SharedSteadyComponentTest(10));
	const SharedSteadyComponentTest *shared_component_id_7_ptr = storage.get_shared_component(shared_component_id_7);

	const godex::SID shared_component_id_8 = storage.create_shared_component(
			SharedSteadyComponentTest(10));
	const SharedSteadyComponentTest *shared_component_id_8_ptr = storage.get_shared_component(shared_component_id_8);

	const godex::SID shared_component_id_9 = storage.create_shared_component(
			SharedSteadyComponentTest(10));
	const SharedSteadyComponentTest *shared_component_id_9_ptr = storage.get_shared_component(shared_component_id_9);

	const godex::SID shared_component_id_10 = storage.create_shared_component(
			SharedSteadyComponentTest(10));
	const SharedSteadyComponentTest *shared_component_id_10_ptr = storage.get_shared_component(shared_component_id_10);

	// Make sure the memory is still the same at this point.
	CHECK(storage.get_shared_component(shared_component_id_1) == shared_component_id_1_ptr);
	CHECK(storage.get_shared_component(shared_component_id_2) == shared_component_id_2_ptr);
	CHECK(storage.get_shared_component(shared_component_id_3) == shared_component_id_3_ptr);
	CHECK(storage.get_shared_component(shared_component_id_4) == shared_component_id_4_ptr);
	CHECK(storage.get_shared_component(shared_component_id_5) == shared_component_id_5_ptr);
	CHECK(storage.get_shared_component(shared_component_id_6) == shared_component_id_6_ptr);
	CHECK(storage.get_shared_component(shared_component_id_7) == shared_component_id_7_ptr);
	CHECK(storage.get_shared_component(shared_component_id_8) == shared_component_id_8_ptr);
	CHECK(storage.get_shared_component(shared_component_id_9) == shared_component_id_9_ptr);
	CHECK(storage.get_shared_component(shared_component_id_10) == shared_component_id_10_ptr);

	// Free the components and check the memory
	{
		storage.free_shared_component(shared_component_id_1);
		CHECK(storage.has_shared_component(shared_component_id_1) == false);
		CHECK(storage.get_shared_component(shared_component_id_2) == shared_component_id_2_ptr);
		CHECK(storage.get_shared_component(shared_component_id_3) == shared_component_id_3_ptr);
		CHECK(storage.get_shared_component(shared_component_id_4) == shared_component_id_4_ptr);
		CHECK(storage.get_shared_component(shared_component_id_5) == shared_component_id_5_ptr);
		CHECK(storage.get_shared_component(shared_component_id_6) == shared_component_id_6_ptr);
		CHECK(storage.get_shared_component(shared_component_id_7) == shared_component_id_7_ptr);
		CHECK(storage.get_shared_component(shared_component_id_8) == shared_component_id_8_ptr);
		CHECK(storage.get_shared_component(shared_component_id_9) == shared_component_id_9_ptr);
		CHECK(storage.get_shared_component(shared_component_id_10) == shared_component_id_10_ptr);

		storage.free_shared_component(shared_component_id_2);
		CHECK(storage.has_shared_component(shared_component_id_1) == false);
		CHECK(storage.has_shared_component(shared_component_id_2) == false);
		CHECK(storage.get_shared_component(shared_component_id_3) == shared_component_id_3_ptr);
		CHECK(storage.get_shared_component(shared_component_id_4) == shared_component_id_4_ptr);
		CHECK(storage.get_shared_component(shared_component_id_5) == shared_component_id_5_ptr);
		CHECK(storage.get_shared_component(shared_component_id_6) == shared_component_id_6_ptr);
		CHECK(storage.get_shared_component(shared_component_id_7) == shared_component_id_7_ptr);
		CHECK(storage.get_shared_component(shared_component_id_8) == shared_component_id_8_ptr);
		CHECK(storage.get_shared_component(shared_component_id_9) == shared_component_id_9_ptr);
		CHECK(storage.get_shared_component(shared_component_id_10) == shared_component_id_10_ptr);

		storage.free_shared_component(shared_component_id_3);
		CHECK(storage.has_shared_component(shared_component_id_1) == false);
		CHECK(storage.has_shared_component(shared_component_id_2) == false);
		CHECK(storage.has_shared_component(shared_component_id_3) == false);
		CHECK(storage.get_shared_component(shared_component_id_4) == shared_component_id_4_ptr);
		CHECK(storage.get_shared_component(shared_component_id_5) == shared_component_id_5_ptr);
		CHECK(storage.get_shared_component(shared_component_id_6) == shared_component_id_6_ptr);
		CHECK(storage.get_shared_component(shared_component_id_7) == shared_component_id_7_ptr);
		CHECK(storage.get_shared_component(shared_component_id_8) == shared_component_id_8_ptr);
		CHECK(storage.get_shared_component(shared_component_id_9) == shared_component_id_9_ptr);
		CHECK(storage.get_shared_component(shared_component_id_10) == shared_component_id_10_ptr);

		storage.free_shared_component(shared_component_id_4);
		CHECK(storage.has_shared_component(shared_component_id_1) == false);
		CHECK(storage.has_shared_component(shared_component_id_2) == false);
		CHECK(storage.has_shared_component(shared_component_id_3) == false);
		CHECK(storage.has_shared_component(shared_component_id_4) == false);
		CHECK(storage.get_shared_component(shared_component_id_5) == shared_component_id_5_ptr);
		CHECK(storage.get_shared_component(shared_component_id_6) == shared_component_id_6_ptr);
		CHECK(storage.get_shared_component(shared_component_id_7) == shared_component_id_7_ptr);
		CHECK(storage.get_shared_component(shared_component_id_8) == shared_component_id_8_ptr);
		CHECK(storage.get_shared_component(shared_component_id_9) == shared_component_id_9_ptr);
		CHECK(storage.get_shared_component(shared_component_id_10) == shared_component_id_10_ptr);

		storage.free_shared_component(shared_component_id_5);
		CHECK(storage.has_shared_component(shared_component_id_1) == false);
		CHECK(storage.has_shared_component(shared_component_id_2) == false);
		CHECK(storage.has_shared_component(shared_component_id_3) == false);
		CHECK(storage.has_shared_component(shared_component_id_4) == false);
		CHECK(storage.has_shared_component(shared_component_id_5) == false);
		CHECK(storage.get_shared_component(shared_component_id_6) == shared_component_id_6_ptr);
		CHECK(storage.get_shared_component(shared_component_id_7) == shared_component_id_7_ptr);
		CHECK(storage.get_shared_component(shared_component_id_8) == shared_component_id_8_ptr);
		CHECK(storage.get_shared_component(shared_component_id_9) == shared_component_id_9_ptr);
		CHECK(storage.get_shared_component(shared_component_id_10) == shared_component_id_10_ptr);

		storage.free_shared_component(shared_component_id_6);
		CHECK(storage.has_shared_component(shared_component_id_1) == false);
		CHECK(storage.has_shared_component(shared_component_id_2) == false);
		CHECK(storage.has_shared_component(shared_component_id_3) == false);
		CHECK(storage.has_shared_component(shared_component_id_4) == false);
		CHECK(storage.has_shared_component(shared_component_id_5) == false);
		CHECK(storage.has_shared_component(shared_component_id_6) == false);
		CHECK(storage.get_shared_component(shared_component_id_7) == shared_component_id_7_ptr);
		CHECK(storage.get_shared_component(shared_component_id_8) == shared_component_id_8_ptr);
		CHECK(storage.get_shared_component(shared_component_id_9) == shared_component_id_9_ptr);
		CHECK(storage.get_shared_component(shared_component_id_10) == shared_component_id_10_ptr);

		storage.free_shared_component(shared_component_id_7);
		CHECK(storage.has_shared_component(shared_component_id_1) == false);
		CHECK(storage.has_shared_component(shared_component_id_2) == false);
		CHECK(storage.has_shared_component(shared_component_id_3) == false);
		CHECK(storage.has_shared_component(shared_component_id_4) == false);
		CHECK(storage.has_shared_component(shared_component_id_5) == false);
		CHECK(storage.has_shared_component(shared_component_id_6) == false);
		CHECK(storage.has_shared_component(shared_component_id_7) == false);
		CHECK(storage.get_shared_component(shared_component_id_8) == shared_component_id_8_ptr);
		CHECK(storage.get_shared_component(shared_component_id_9) == shared_component_id_9_ptr);
		CHECK(storage.get_shared_component(shared_component_id_10) == shared_component_id_10_ptr);

		storage.free_shared_component(shared_component_id_8);
		CHECK(storage.has_shared_component(shared_component_id_1) == false);
		CHECK(storage.has_shared_component(shared_component_id_2) == false);
		CHECK(storage.has_shared_component(shared_component_id_3) == false);
		CHECK(storage.has_shared_component(shared_component_id_4) == false);
		CHECK(storage.has_shared_component(shared_component_id_5) == false);
		CHECK(storage.has_shared_component(shared_component_id_6) == false);
		CHECK(storage.has_shared_component(shared_component_id_7) == false);
		CHECK(storage.has_shared_component(shared_component_id_8) == false);
		CHECK(storage.get_shared_component(shared_component_id_9) == shared_component_id_9_ptr);
		CHECK(storage.get_shared_component(shared_component_id_10) == shared_component_id_10_ptr);

		storage.free_shared_component(shared_component_id_9);
		CHECK(storage.has_shared_component(shared_component_id_1) == false);
		CHECK(storage.has_shared_component(shared_component_id_2) == false);
		CHECK(storage.has_shared_component(shared_component_id_3) == false);
		CHECK(storage.has_shared_component(shared_component_id_4) == false);
		CHECK(storage.has_shared_component(shared_component_id_5) == false);
		CHECK(storage.has_shared_component(shared_component_id_6) == false);
		CHECK(storage.has_shared_component(shared_component_id_7) == false);
		CHECK(storage.has_shared_component(shared_component_id_8) == false);
		CHECK(storage.has_shared_component(shared_component_id_9) == false);
		CHECK(storage.get_shared_component(shared_component_id_10) == shared_component_id_10_ptr);
	}

	// Check clear.
	storage.clear();
	CHECK(storage.has_shared_component(shared_component_id_1) == false);
	CHECK(storage.has_shared_component(shared_component_id_2) == false);
	CHECK(storage.has_shared_component(shared_component_id_3) == false);
	CHECK(storage.has_shared_component(shared_component_id_4) == false);
	CHECK(storage.has_shared_component(shared_component_id_5) == false);
	CHECK(storage.has_shared_component(shared_component_id_6) == false);
	CHECK(storage.has_shared_component(shared_component_id_7) == false);
	CHECK(storage.has_shared_component(shared_component_id_8) == false);
	CHECK(storage.has_shared_component(shared_component_id_9) == false);
	CHECK(storage.has_shared_component(shared_component_id_10) == false);
}
} // namespace godex_ecs_shared_steady_storage_tests

struct SharedComponentTest2 {
	COMPONENT(SharedComponentTest2, SharedSteadyStorage)

	static void _bind_methods() {
		ECS_BIND_PROPERTY(SharedComponentTest2, PropertyInfo(Variant::INT, "number"), number);
	}

	int number = 0;
};

namespace godex_ecs_shared_steady_storage_tests {
TEST_CASE("[SharedSteadyStorage] Check World storage usage.") {
	ECS::register_component<SharedComponentTest2>();

	World world;

	Dictionary sc_1;
	sc_1["number"] = 10;
	const godex::SID sid_1 = world.create_shared_component(SharedComponentTest2::get_component_id(), sc_1);

	SharedComponentTest2 sct_2;
	sct_2.number = 100;
	const godex::SID sid_2 = world.create_shared_component<SharedComponentTest2>(sct_2);

	const EntityID entity_1 = world
									  .create_entity()
									  .with<SharedComponentTest2>(sid_1);

	const EntityID entity_2 = world
									  .create_entity()
									  .with<SharedComponentTest2>(sid_1);

	const EntityID entity_3 = world
									  .create_entity()
									  .with<SharedComponentTest2>(sid_2);

	const EntityID entity_4 = world
									  .create_entity()
									  .with<SharedComponentTest2>(sid_2);

	// Verify the storage has the entities.
	{
		SharedStorage<SharedComponentTest2> *storage = world.get_shared_storage<SharedComponentTest2>();
		CHECK(storage->has(entity_1));
		CHECK(storage->has(entity_2));
		CHECK(storage->has(entity_3));
		CHECK(storage->has(entity_4));

		CHECK(storage->has_shared_component(sid_1));
		CHECK(storage->has_shared_component(sid_2));
	}

	// Use also the Base, so to confirm we can use it.
	{
		SharedStorageBase *storage = world.get_shared_storage(SharedComponentTest2::get_component_id());
		CHECK(storage->has_shared_component(sid_1));
		CHECK(storage->has_shared_component(sid_2));
	}

	// Make sure the shared component have the desired data.
	{
		SharedStorage<SharedComponentTest2> *storage = world.get_shared_storage<SharedComponentTest2>();
		SharedComponentTest2 *shared_comp_1 = storage->get_shared_component(sid_1);
		SharedComponentTest2 *shared_comp_2 = storage->get_shared_component(sid_2);

		CHECK(shared_comp_1->number == 10);
		CHECK(shared_comp_2->number == 100);
	}

	// Check if fetch works fine and the sharing works too.
	{
		Storage<SharedComponentTest2> *storage = world.get_storage<SharedComponentTest2>();
		CHECK(storage->get(entity_1) == storage->get(entity_2));
		CHECK(storage->get(entity_3) == storage->get(entity_4));
		CHECK(storage->get(entity_3) == storage->get(entity_4));

		CHECK(storage->get(entity_1)->number == 10);
		CHECK(storage->get(entity_3)->number == 100);
	}

	// Try to mutate one and make sure the change is propagated.
	{
		Storage<SharedComponentTest2> *storage = world.get_storage<SharedComponentTest2>();
		storage->get(entity_1)->number = 11;
		storage->get(entity_3)->number = 101;

		CHECK(storage->get(entity_1)->number == 11);
		CHECK(storage->get(entity_2)->number == 11);
		CHECK(storage->get(entity_3)->number == 101);
		CHECK(storage->get(entity_4)->number == 101);
	}

	// Try to mutate using `SID`
	{
		SharedStorage<SharedComponentTest2> *storage = world.get_shared_storage<SharedComponentTest2>();
		SharedComponentTest2 *shared_comp_1 = storage->get_shared_component(sid_1);
		SharedComponentTest2 *shared_comp_2 = storage->get_shared_component(sid_2);

		CHECK(shared_comp_1->number == 11);
		CHECK(shared_comp_2->number == 101);

		shared_comp_1->number = 12;
		shared_comp_2->number = 102;
	}

	// Now validate the change is propagated.
	{
		Storage<SharedComponentTest2> *storage = world.get_storage<SharedComponentTest2>();
		CHECK(storage->get(entity_1)->number == 12);
		CHECK(storage->get(entity_2)->number == 12);
		CHECK(storage->get(entity_3)->number == 102);
		CHECK(storage->get(entity_4)->number == 102);
	}
}
} // namespace godex_ecs_shared_steady_storage_tests

#endif // TEST_SHARED_STEADY_STORAGE_H
