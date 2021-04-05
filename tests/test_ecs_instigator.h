#ifndef TEST_ECS_INSTIGATOR_H
#define TEST_ECS_INSTIGATOR_H

#include "tests/test_macros.h"

#include "../components/component.h"
#include "../ecs.h"
#include "../instigators/instigator.h"
#include "../modules/godot/nodes/ecs_utilities.h"
#include "../storage/dense_vector_storage.h"
#include "test_utilities.h"

struct MyTestEvent {
	INSTIGATOR(MyTestEvent)
};

struct MyInstigatedComponent {
	COMPONENT(MyInstigatedComponent, DenseVectorStorage)
	INSTIGATED(MyTestEvent)
};

namespace godex_tests {

TEST_CASE("[Modules][ECS] Test instigator C++ registration.") {
	ECS::register_instigator<MyTestEvent>();

	{
		const LocalVector<godex::component_id> &instigated = ECS::get_instigated_components(MyTestEvent::get_instigator_id());
		CHECK(instigated.size() == 0);
	}

	ECS::register_component<MyInstigatedComponent>();

	{
		const LocalVector<godex::instigator_id> &instigators = ECS::get_instigators(MyInstigatedComponent::get_component_id());
		CHECK(instigators.size() == 1);
		CHECK(instigators[0] == MyTestEvent::get_instigator_id());
	}

	{
		const LocalVector<godex::component_id> &instigated = ECS::get_instigated_components(MyTestEvent::get_instigator_id());
		CHECK(instigated.size() == 1);
		CHECK(instigated[0] == MyInstigatedComponent::get_component_id());
	}
}

TEST_CASE("[Modules][ECS] Test instigator Script registration.") {
	// Compose the script component
	Ref<Component> component;

	{
		Object target_obj;

		// Create the script.
		String code;
		code += "extends Component\n";
		code += "\n";
		code += "func get_instigators():\n";
		code += "	return ['MyTestEvent']\n";
		code += "\n";
		code += "var aa: int = 0\n";
		code += "var bb: int = 0\n";
		code += "\n";

		CHECK(build_and_assign_script(&target_obj, code));

		const String err = Component::validate_script(target_obj.get_script());
		CHECK(err == "");

		component.instance();
		component->internal_set_name("MyScriptInstigatedComponent.gd");
		component->internal_set_component_script(target_obj.get_script());
	}

	// Make sure the instigators extractions is working
	{
		CHECK(component.is_valid());
		Vector<StringName> instigators = component->get_instigators();
		CHECK(instigators.size() == 1);
		CHECK(instigators[0] == StringName("MyTestEvent"));
	}

	// Register the component.
	EditorEcs::register_dynamic_component(component.ptr());

	// Make sure the instigators are correctly set during component initialization.
	{
		const LocalVector<godex::instigator_id> &instigators = ECS::get_instigators(ECS::get_component_id(component->get_name()));
		CHECK(instigators.size() == 1);
		CHECK(instigators[0] == MyTestEvent::get_instigator_id());

		const LocalVector<godex::component_id> &instigated = ECS::get_instigated_components(MyTestEvent::get_instigator_id());
		CHECK(instigated.size() == 2);
		CHECK(instigated.find(MyInstigatedComponent::get_component_id()) != -1);
		CHECK(instigated.find(ECS::get_component_id(component->get_name())) != -1);
	}
}
} // namespace godex_tests

#endif // TEST_ECS_BASE_H
