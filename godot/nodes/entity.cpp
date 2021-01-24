
#include "entity.h"

#include "ecs_utilities.h"
#include "ecs_world.h"

// TODO this file is full of `get_component_id`. Would be nice cache it.

template <class C>
bool EntityInternal<C>::_set(const StringName &p_name, const Variant &p_value) {
	const Vector<String> names = String(p_name).split("/");
	if (names.size() == 1) {
		return set_component(p_name, p_value);
	} else {
		ERR_FAIL_COND_V(names.size() < 2, false);
		const String component_name = names[0];
		const String property_name = names[1];

		return set_component_value(component_name, property_name, p_value);
	}
}

template <class C>
bool EntityInternal<C>::_get(const StringName &p_name, Variant &r_ret) const {
	const Vector<String> names = String(p_name).split("/");
	if (names.size() == 1) {
		return _get_component(p_name, r_ret);
	} else {
		ERR_FAIL_COND_V(names.size() < 2, false);
		const String component_name = names[0];
		const String property_name = names[1];

		return _get_component_value(component_name, property_name, r_ret);
	}
}

template <class C>
void EntityInternal<C>::_notification(int p_what) {
	switch (p_what) {
		case Node::NOTIFICATION_READY:
			if (Engine::get_singleton()->is_editor_hint() == false) {
#ifdef TOOLS_ENABLED
				// At this point the entity is never created.
				CRASH_COND(entity_id.is_null() == false);
#endif
				ECS::get_singleton()->connect("world_loaded", callable_mp(owner, &C::create_entity));
				ECS::get_singleton()->connect("world_pre_unload", callable_mp(owner, &C::destroy_entity));
				create_entity();
			}
			break;
		case Node::NOTIFICATION_EXIT_TREE:
			if (Engine::get_singleton()->is_editor_hint() == false) {
				ECS::get_singleton()->disconnect("world_loaded", callable_mp(owner, &C::create_entity));
				ECS::get_singleton()->disconnect("world_pre_unload", callable_mp(owner, &C::destroy_entity));
				destroy_entity();
			}
			break;
	}
}

template <class C>
void EntityInternal<C>::add_component(const StringName &p_component_name, const Dictionary &p_values) {
	if (entity_id.is_null()) {
		components_data[p_component_name] = p_values;
		update_components_data();
		owner->update_gizmo();
	} else {
		const godex::component_id id = ECS::get_component_id(p_component_name);
		ERR_FAIL_COND_MSG(id == UINT32_MAX, "The component " + p_component_name + " doesn't exists.");
		ERR_FAIL_COND_MSG(ECS::get_singleton()->has_active_world() == false, "The world is supposed to be active at this point.");
		ECS::get_singleton()->get_active_world()->add_component(entity_id, id, p_values);
	}
}

template <class C>
void EntityInternal<C>::remove_component(const StringName &p_component_name) {
	if (entity_id.is_null()) {
		components_data.erase(p_component_name);
		update_components_data();
		owner->update_gizmo();
	} else {
		const godex::component_id id = ECS::get_component_id(p_component_name);
		ERR_FAIL_COND_MSG(id == UINT32_MAX, "The component " + p_component_name + " doesn't exists.");
		ERR_FAIL_COND_MSG(ECS::get_singleton()->has_active_world() == false, "The world is supposed to be active at this point.");
		ECS::get_singleton()->get_active_world()->remove_component(entity_id, id);
	}
}

template <class C>
bool EntityInternal<C>::has_component(const StringName &p_component_name) const {
	if (entity_id.is_null()) {
		return components_data.has(p_component_name);
	} else {
		const godex::component_id id = ECS::get_component_id(p_component_name);
		ERR_FAIL_COND_V_MSG(id == UINT32_MAX, false, "The component " + p_component_name + " doesn't exists.");
		ERR_FAIL_COND_V_MSG(ECS::get_singleton()->has_active_world() == false, false, "The world is supposed to be active at this point.");
		return ECS::get_singleton()->get_active_world()->has_component(entity_id, id);
	}
}

template <class C>
void EntityInternal<C>::set_components_data(Dictionary p_data) {
	components_data = p_data;
	update_components_data();
}

template <class C>
const Dictionary &EntityInternal<C>::get_components_data() const {
	return components_data;
}

template <class C>
bool EntityInternal<C>::set_component_value(const StringName &p_component_name, const StringName &p_property_name, const Variant &p_value) {
	if (entity_id.is_null()) {
		ERR_FAIL_COND_V(components_data.has(p_component_name) == false, false);
		if (components_data[p_component_name].get_type() != Variant::DICTIONARY) {
			components_data[p_component_name] = Dictionary();
		}
		(components_data[p_component_name].operator Dictionary())[p_property_name] = p_value;
		print_line("Component " + p_component_name + " property " + p_property_name + " changed to " + p_value);
		update_components_data();
		return true;
	} else {
		// This entity exist, so set it on the World.

		ERR_FAIL_COND_V_MSG(ECS::get_singleton()->has_active_world() == false, false, "The world is supposed to be active at this point.");
		const godex::component_id id = ECS::get_component_id(p_component_name);
		ERR_FAIL_COND_V_MSG(id == UINT32_MAX, false, "The component " + p_component_name + " doesn't exists.");
		StorageBase *storage = ECS::get_singleton()->get_active_world()->get_storage(id);
		ERR_FAIL_COND_V_MSG(storage == nullptr, false, "The component " + p_component_name + " doesn't have a storage on the active world.");
		godex::Component *component = storage->get_ptr(entity_id);
		ERR_FAIL_COND_V_MSG(component == nullptr, false, "The entity " + itos(entity_id) + " doesn't have a component " + p_component_name);
		component->set(p_property_name, p_value);
		return true;
	}
}

template <class C>
Variant EntityInternal<C>::get_component_value(const StringName &p_component_name, const StringName &p_property_name) const {
	Variant ret;
	// No need to test if success because the error is already logged.
	_get_component_value(p_component_name, p_property_name, ret);
	return ret;
}

template <class C>
bool EntityInternal<C>::_get_component_value(const StringName &p_component_name, const StringName &p_property_name, Variant &r_ret) const {
	// This function is always executed in single thread.

	if (entity_id.is_null()) {
		// Executed on editor or when the entity is not loaded.

		const Variant *component_properties = components_data.getptr(p_component_name);
		ERR_FAIL_COND_V_MSG(component_properties == nullptr, false, "The component " + p_component_name + " doesn't exist on this entity: " + get_path());

		if (component_properties->get_type() == Variant::DICTIONARY) {
			const Variant *value = (component_properties->operator Dictionary()).getptr(p_property_name);
			if (value != nullptr) {
				// Property is stored, just return it.
				r_ret = *value;
				return true;
			}
		}

		// Property was not found, take the default one.
		if (String(p_component_name).ends_with(".gd")) {
			// This is a Script Component.
			const uint32_t id = ScriptECS::get_component_id(p_component_name);
			ERR_FAIL_COND_V_MSG(id == UINT32_MAX, false, "The script component " + p_component_name + " was not found.");
			Ref<Component> c = ScriptECS::get_component(id);
			r_ret = c->get_property_default_value(p_property_name);
			return true;
		} else {
			// This is a native Component.
			const godex::component_id id = ECS::get_component_id(p_component_name);
			ERR_FAIL_COND_V_MSG(id == UINT32_MAX, false, "The component " + p_component_name + " doesn't exists.");
			r_ret = ECS::get_component_property_default(id, p_property_name);
			return true;
		}
	} else {
		// This entity exist on a world, check the value on the world storage.

		ERR_FAIL_COND_V_MSG(ECS::get_singleton()->has_active_world() == false, false, "The world is supposed to be active at this point.");
		const godex::component_id id = ECS::get_component_id(p_component_name);
		ERR_FAIL_COND_V_MSG(id == UINT32_MAX, false, "The component " + p_component_name + " doesn't exists.");
		const StorageBase *storage = ECS::get_singleton()->get_active_world()->get_storage(id);
		ERR_FAIL_COND_V_MSG(storage == nullptr, false, "The component " + p_component_name + " doesn't have a storage on the active world.");
		const godex::Component *component = storage->get_ptr(entity_id);
		ERR_FAIL_COND_V_MSG(component == nullptr, false, "The entity " + itos(entity_id) + " doesn't have a component " + p_component_name);
		return component->get(p_property_name, r_ret);
	}
}

template <class C>
bool EntityInternal<C>::set_component(const StringName &p_component_name, const Variant &d_data) {
	Dictionary data = d_data;

	if (entity_id.is_null()) {
		const Variant *component_properties = components_data.getptr(p_component_name);
		if (component_properties == nullptr) {
			components_data[p_component_name] = Dictionary();
			component_properties = components_data.getptr(p_component_name);
		}
		for (const Variant *key = data.next(nullptr); key != nullptr; key = data.next(key)) {
			(component_properties->operator Dictionary())[*key] = *data.getptr(*key);
		}
		return true;
	} else {
		// This entity exist on a world, just fetch the component.

		ERR_FAIL_COND_V_MSG(ECS::get_singleton()->has_active_world() == false, false, "The world is supposed to be active at this point.");
		const godex::component_id id = ECS::get_component_id(p_component_name);
		ERR_FAIL_COND_V_MSG(id == UINT32_MAX, false, "The component " + p_component_name + " doesn't exists.");
		StorageBase *storage = ECS::get_singleton()->get_active_world()->get_storage(id);
		ERR_FAIL_COND_V_MSG(storage == nullptr, false, "The component " + p_component_name + " doesn't have a storage on the active world.");
		godex::Component *component = storage->get_ptr(entity_id);
		ERR_FAIL_COND_V_MSG(component == nullptr, false, "The entity " + itos(entity_id) + " doesn't have a component " + p_component_name);

		for (const Variant *key = data.next(nullptr); key != nullptr; key = data.next(key)) {
			component->set(StringName(*key), *data.getptr(*key));
		}
		return true;
	}
}

template <class C>
bool EntityInternal<C>::_get_component(const StringName &p_component_name, Variant &r_ret) const {
	if (entity_id.is_null()) {
		// Entity is null, so take default or set what we have in `component_data`.
		Dictionary dic;

		const Variant *component_properties = components_data.getptr(p_component_name);
		ERR_FAIL_COND_V_MSG(component_properties == nullptr, Variant(), "The component " + p_component_name + " doesn't exist on this entity: " + get_path());

		if (String(p_component_name).ends_with(".gd")) {
			// TODO this is simply not optimal.

			// This is a Script Component.
			const uint32_t id = ScriptECS::get_component_id(p_component_name);
			ERR_FAIL_COND_V_MSG(id == UINT32_MAX, false, "The script component " + p_component_name + " was not found.");
			Ref<Component> c = ScriptECS::get_component(id);
			List<PropertyInfo> properties;
			c->get_component_property_list(&properties);

			for (List<PropertyInfo>::Element *e = properties.front(); e; e = e->next()) {
				const Variant *value = (component_properties->operator Dictionary()).getptr(e->get().name);
				if (value) {
					// Just set the value in the data.
					dic[e->get().name] = *value;
				} else {
					// TODO Instead this is extremely bad.
					// Just take the default value.
					dic[e->get().name] = c->get_property_default_value(e->get().name);
				}
			}

		} else {
			// This is a native Component.

			const godex::component_id id = ECS::get_component_id(p_component_name);
			ERR_FAIL_COND_V_MSG(id == UINT32_MAX, false, "The component " + p_component_name + " doesn't exists.");
			const LocalVector<PropertyInfo> *properties = ECS::get_component_properties(id);
			for (uint32_t i = 0; i < properties->size(); i += 1) {
				const Variant *value = (component_properties->operator Dictionary()).getptr((*properties)[i].name);
				if (value) {
					// Just set the value in the data.
					dic[(*properties)[i].name] = *value;
				} else {
					// TODO Instead this is extremely bad.
					// Just take the default value.
					dic[(*properties)[i].name] = ECS::get_component_property_default(id, (*properties)[i].name);
				}
			}
		}

		r_ret = dic;
		return true;

	} else {
		// This entity exist on a world, just fetch the component.

		ERR_FAIL_COND_V_MSG(ECS::get_singleton()->has_active_world() == false, false, "The world is supposed to be active at this point.");
		const godex::component_id id = ECS::get_component_id(p_component_name);
		ERR_FAIL_COND_V_MSG(id == UINT32_MAX, false, "The component " + p_component_name + " doesn't exists.");
		const StorageBase *storage = ECS::get_singleton()->get_active_world()->get_storage(id);
		ERR_FAIL_COND_V_MSG(storage == nullptr, false, "The component " + p_component_name + " doesn't have a storage on the active world.");
		const godex::Component *component = storage->get_ptr(entity_id);
		ERR_FAIL_COND_V_MSG(component == nullptr, false, "The entity " + itos(entity_id) + " doesn't have a component " + p_component_name);

		Dictionary dic;
		Variant supp;
		for (uint32_t i = 0; i < component->get_properties()->size(); i += 1) {
			if (likely(component->get(i, supp))) {
				dic[(*component->get_properties())[i].name] = supp;
			} else {
				dic[(*component->get_properties())[i].name] = Variant();
			}
		}

		r_ret = dic;
		return true;
	}
}

template <class C>
uint32_t EntityInternal<C>::clone(Object *p_world) const {
	WorldECS *world = Object::cast_to<WorldECS>(p_world);
	ERR_FAIL_COND_V_MSG(world == nullptr, EntityID(), "The passed object is not a `WorldECS`.");
	ERR_FAIL_COND_V_MSG(world->get_world() == nullptr, EntityID(), "This world doesn't have the ECS world.");
	return _create_entity(world->get_world());
}

template <class C>
void EntityInternal<C>::create_entity() {
	if (entity_id.is_null() == false) {
		// Nothing to do.
		return;
	}

	if (ECS::get_singleton()->has_active_world()) {
		// It's safe dereference command because this function is always called
		// when the world is not dispatching.
		entity_id = _create_entity(ECS::get_singleton()->get_active_world());
	}
}

template <class C>
EntityID EntityInternal<C>::_create_entity(World *p_world) const {
	EntityID id;
	if (p_world) {
		id = p_world->create_entity();

		for (
				const Variant *key = components_data.next(nullptr);
				key != nullptr;
				key = components_data.next(key)) {
			const uint32_t component_id = ECS::get_component_id(*key);
			ERR_CONTINUE(component_id == UINT32_MAX);
			p_world->add_component(
					id,
					component_id,
					*components_data.getptr(*key));
		}
	}
	return id;
}

template <class C>
void EntityInternal<C>::destroy_entity() {
	if (entity_id.is_null()) {
		// Nothing to do.
		return;
	}

	if (ECS::get_singleton()->has_active_world()) {
		// It's safe dereference command because this function is always called
		// when the world is not dispatching.
		ECS::get_singleton()->get_active_world()->destroy_entity(entity_id);
	}

	entity_id = EntityID();
}

template <class C>
void EntityInternal<C>::update_components_data() {
	owner->_change_notify("components_data");
}

void Entity3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("__set_components_data", "data"), &Entity3D::set_components_data);
	ClassDB::bind_method(D_METHOD("__get_components_data"), &Entity3D::get_components_data);

	ClassDB::bind_method(D_METHOD("add_component", "component_name", "values"), &Entity3D::add_component, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("remove_component", "component_name"), &Entity3D::remove_component);
	ClassDB::bind_method(D_METHOD("has_component", "component_name"), &Entity3D::has_component);

	ClassDB::bind_method(D_METHOD("set_component_value", "component", "property", "value"), &Entity3D::set_component_value);
	ClassDB::bind_method(D_METHOD("get_component_value", "component", "property"), &Entity3D::get_component_value);

	ClassDB::bind_method(D_METHOD("clone", "world"), &Entity3D::clone);

	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "__component_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "__set_components_data", "__get_components_data");
}

void Entity2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("__set_components_data", "data"), &Entity2D::set_components_data);
	ClassDB::bind_method(D_METHOD("__get_components_data"), &Entity2D::get_components_data);

	ClassDB::bind_method(D_METHOD("add_component", "component_name", "values"), &Entity2D::add_component, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("remove_component", "component_name"), &Entity2D::remove_component);
	ClassDB::bind_method(D_METHOD("has_component", "component_name"), &Entity2D::has_component);

	ClassDB::bind_method(D_METHOD("set_component_value", "component", "property", "value"), &Entity2D::set_component_value);
	ClassDB::bind_method(D_METHOD("get_component_value", "component", "property"), &Entity2D::get_component_value);

	ClassDB::bind_method(D_METHOD("clone", "world"), &Entity2D::clone);

	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "__component_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "__set_components_data", "__get_components_data");
}
