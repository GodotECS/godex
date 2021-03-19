#include "shared_component_resource.h"

void SharedComponentResource::_bind_methods() {
	ClassDB::bind_method(D_METHOD("__set_component_name", "name"), &SharedComponentResource::set_component_name);
	ClassDB::bind_method(D_METHOD("__get_component_name"), &SharedComponentResource::get_component_name);

	ClassDB::bind_method(D_METHOD("__set_component_data", "data"), &SharedComponentResource::set_component_data);
	ClassDB::bind_method(D_METHOD("__get_component_data"), &SharedComponentResource::get_component_data);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "component_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "__set_component_name", "__set_component_name");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "component_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "__set_component_data", "__set_component_data");
}

SharedComponentResource::SharedComponentResource() {
}

void SharedComponentResource::init(const StringName &p_component_name) {
	ERR_FAIL_COND_MSG(component_name != StringName(), "This SharedComponentResource is already initialized. You can't do it twice.");

	const godex::component_id component_id = ECS::get_component_id(p_component_name);
	ERR_FAIL_COND_MSG(ECS::verify_component_id(component_id) == false, "You can't initialize a SharedComponentResource with an invalid component id: " + p_component_name);
	ERR_FAIL_COND_MSG(ECS::is_component_sharable(component_id) == false, "It's not possible to initialize a SharedComponentResource for a component that is not shareable.");

	component_name = p_component_name;
}

/// Returns the `SID` for this world. Creates one if it has none yet.
godex::SID SharedComponentResource::get_sid(World *p_world) {
	// Try to see if we already have the Shared ID.
	const int64_t index = sids.find({ p_world, godex::SID_NONE });
	if (index != -1) {
		return sids[index].sid;
	} else {
		const godex::component_id component_id = ECS::get_component_id(component_name);
		ERR_FAIL_COND_V_MSG(ECS::verify_component_id(component_id) == false, godex::SID_NONE, "You can't get a Shared ID from an invalid component. Component name: '" + component_name + "'");
		ERR_FAIL_COND_V_MSG(ECS::is_component_sharable(component_id) == false, godex::SID_NONE, "This component is not sharable. You can't obtain a Shared ID. Component name: '" + component_name + "'");

		// Make sure the storage exist.
		p_world->create_storage(component_id);

		// Take the storage, we can use static_cast because `init` make sure
		// this component is using a `SharedStorage`.
		SharedStorageBase *storage = (SharedStorageBase *)p_world->get_storage(component_id);

		// Create the component.
		const godex::SID sid = storage->create_shared_component_dynamic(component_data);
		sids.push_back({ p_world, sid });

		return sid;
	}
}

void SharedComponentResource::set_component_name(const StringName &p_component_name) {
	component_name = p_component_name;
}

StringName SharedComponentResource::get_component_name() const {
	return component_name;
}

void SharedComponentResource::set_component_data(const Dictionary &p_component) {
	component_data = p_component;
}

Dictionary SharedComponentResource::get_component_data() const {
	return component_data;
}

Ref<Resource> SharedComponentResource::duplicate(bool p_subresources) const {
	Ref<SharedComponentResource> res = Resource::duplicate(p_subresources);

	// Make sure this new resource doesn't have any assigned SID.
	res->sids.clear();

	// `component_name` & `component_data` are already duplicated at this point.

	return res;
}
