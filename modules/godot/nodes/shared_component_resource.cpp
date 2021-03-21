#include "shared_component_resource.h"

#include "ecs_utilities.h"

void SharedComponentResource::_bind_methods() {
	ClassDB::bind_method(D_METHOD("__set_component_name", "name"), &SharedComponentResource::set_component_name);
	ClassDB::bind_method(D_METHOD("__get_component_name"), &SharedComponentResource::get_component_name);

	ClassDB::bind_method(D_METHOD("__set_component_data", "data"), &SharedComponentResource::set_component_data);
	ClassDB::bind_method(D_METHOD("__get_component_data"), &SharedComponentResource::get_component_data);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "component_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "__set_component_name", "__get_component_name");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "component_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "__set_component_data", "__get_component_data");
}

bool SharedComponentResource::_set(const StringName &p_name, const Variant &p_property) {
	ERR_FAIL_COND_V_MSG(is_init() == false, false, "This shared component is not init. So you can't use it.");

	component_data[p_name] = p_property;
	return true;
}

bool SharedComponentResource::_get(const StringName &p_name, Variant &r_property) const {
	ERR_FAIL_COND_V_MSG(is_init() == false, false, "This shared component is not init. So you can't use it.");

	if (const Variant *v = component_data.getptr(p_name)) {
		r_property = *v;
		return true;
	} else {
		// Data not found, try to take the default value.
		return EditorEcs::component_get_property_default_value(component_name, p_name, r_property);
	}
}

void SharedComponentResource::_get_property_list(List<PropertyInfo> *p_list) const {
	ERR_FAIL_COND_MSG(is_init() == false, "This shared component is not init. So you can't use it.");
	ERR_FAIL_COND_MSG(EditorEcs::component_is_shared(component_name) == false, "This component is not shared, this is not supposed to happen.");

	EditorEcs::component_get_properties(component_name, p_list);
}

SharedComponentResource::SharedComponentResource() {
}

void SharedComponentResource::init(const StringName &p_component_name) {
	ERR_FAIL_COND_MSG(is_init(), "This SharedComponentResource is already initialized. You can't do it twice.");

	const godex::component_id component_id = ECS::get_component_id(p_component_name);
	ERR_FAIL_COND_MSG(ECS::verify_component_id(component_id) == false, "You can't initialize a SharedComponentResource with an invalid component id: " + p_component_name);
	ERR_FAIL_COND_MSG(ECS::is_component_sharable(component_id) == false, "It's not possible to initialize a SharedComponentResource for a component that is not shareable.");

	component_name = p_component_name;
}

bool SharedComponentResource::is_init() const {
	return component_name != StringName();
}

/// Returns the `SID` for this world. Creates one if it has none yet.
godex::SID SharedComponentResource::get_sid(World *p_world) {
	// Try to see if we already have the Shared ID.
	const int64_t index = sids.find({ p_world, godex::SID_NONE });
	if (index != -1) {
		return sids[index].sid;
	} else {
		const godex::component_id component_id = ECS::get_component_id(component_name);
		const godex::SID sid = p_world->create_shared_component(component_id, component_data);

		ERR_FAIL_COND_V(sid == godex::SID_NONE, godex::SID_NONE);

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
