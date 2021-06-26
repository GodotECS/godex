#include "shared_component_resource.h"

#include "ecs_utilities.h"
#include "script_ecs.h"

void SharedComponentResource::_bind_methods() {
	ClassDB::bind_method(D_METHOD("__set_component_name", "name"), &SharedComponentResource::set_component_name);
	ClassDB::bind_method(D_METHOD("__get_component_name"), &SharedComponentResource::get_component_name);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "component_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "__set_component_name", "__get_component_name");
}

bool SharedComponentResource::_set(const StringName &p_name, const Variant &p_property) {
	ERR_FAIL_COND_V_MSG(is_init() == false, false, "This shared component is not init. So you can't use it.");

	bool success = false;
	depot->set(p_name, p_property, &success);
	return success;
}

bool SharedComponentResource::_get(const StringName &p_name, Variant &r_property) const {
	ERR_FAIL_COND_V_MSG(is_init() == false, false, "This shared component is not init. So you can't use it.");

	bool success = false;
	r_property = depot->get(p_name, &success);
	return success;
}

void SharedComponentResource::_get_property_list(List<PropertyInfo> *p_list) const {
	ERR_FAIL_COND_MSG(is_init() == false, "This shared component is not init. So you can't use it.");
	ERR_FAIL_COND_MSG(ECS::is_component_sharable(ECS::get_component_id(component_name)) == false, "This component is not shared, this is not supposed to happen.");

	const godex::component_id id = ECS::get_component_id(component_name);
	ERR_FAIL_COND(id == godex::COMPONENT_NONE);
	const LocalVector<PropertyInfo> *properties = ECS::get_component_properties(id);
	for (uint32_t i = 0; i < properties->size(); i += 1) {
		p_list->push_back((*properties)[i]);
	}
}

SharedComponentResource::SharedComponentResource() {
}

void SharedComponentResource::init(const StringName &p_component_name) {
	ERR_FAIL_COND_MSG(is_init(), "This SharedComponentResource is already initialized. You can't do it twice.");

	const godex::component_id component_id = ECS::get_component_id(p_component_name);
	ERR_FAIL_COND_MSG(ECS::verify_component_id(component_id) == false, "You can't initialize a SharedComponentResource with an invalid component id: " + p_component_name);
	ERR_FAIL_COND_MSG(ECS::is_component_sharable(component_id) == false, "It's not possible to initialize a SharedComponentResource for a component that is not shareable.");

	component_name = p_component_name;
	depot.instantiate();
	depot->init(component_name);
}

bool SharedComponentResource::is_init() const {
	return component_name != StringName();
}

/// Returns the `SID` for this world. Creates one if it has none yet.
godex::SID SharedComponentResource::get_sid(World *p_world) {
	ERR_FAIL_COND_V_MSG(component_name == StringName(), godex::SID_NONE, "This shared component is not yet init.");

	// Try to see if we already have the Shared ID.
	const int64_t index = sids.find({ p_world, godex::SID_NONE });
	if (index != -1) {
		return sids[index].sid;
	} else {
		if (depot.is_null()) {
			depot.instantiate();
			depot->init(component_name);
		}
		const godex::component_id component_id = depot->get_component_id();
		const godex::SID sid = p_world->create_shared_component(component_id, depot->get_properties_data());

		ERR_FAIL_COND_V(sid == godex::SID_NONE, godex::SID_NONE);

		sids.push_back({ p_world, sid });

		return sid;
	}
}

void SharedComponentResource::set_component_name(const StringName &p_component_name) {
	init(p_component_name);
}

StringName SharedComponentResource::get_component_name() const {
	return component_name;
}

Ref<Resource> SharedComponentResource::duplicate(bool p_subresources) const {
	Ref<SharedComponentResource> res = Resource::duplicate(p_subresources);

	// Make sure this new resource doesn't have any assigned SID.
	res->sids.clear();

	// `component_name` & `component_data` are already duplicated at this point.

	return res;
}
