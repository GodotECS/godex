#pragma once

#include "../../../components/child.h"
#include "core/templates/list.h"
#include "core/templates/local_vector.h"
#include "ecs_utilities.h"
#include "ecs_world.h"
#include "scene/2d/node_2d.h"
#include "scene/3d/node_3d.h"
#include "shared_component_resource.h"

class WorldECS;
class World;

/// Class used to store some extra information for the editor.
class ComponentGizmoData : public Reference {
public:
	virtual void on_position_update(const Transform &p_new_transform) {}
};

/// If you know ECS, this setup may sound strange to you, and indeed it's
/// strange. The entities don't have type.
///
/// The `Entity` should just extend `Node`, and we should not have two versions:
/// however, I would not be able to add the gizmos. To add a new 3D gizmo on the
/// Godot Editor, it's necessary that the node for which this gizmo is created
/// extends the `Node3D` (same for 2D).
///
/// Change the editor, so that any node type can add a 3D (2D) gizmo, is
/// possible and easy. However, the process to upstream this change require
/// time, so to speed up the process and so be able to already use the Gizmos
/// I've decided to go this route: Have `Entity3D` and `Entity2D`.
///
/// While this solution is not conceptually correct, it works and doesn't
/// constraint (or forces) anything. So bear in mind that there is no difference
/// between those, _and in future this will be improved_.
struct EntityBase {
#ifdef TOOLS_ENABLED
	// Used by the editor to display meshes and similar.
	OAHashMap<StringName, Ref<ComponentGizmoData>> gizmo_data;
#endif

	EntityID entity_id;
	OAHashMap<StringName, Variant> components_data;

	EntityBase *get_node_entity_base(Node *p_node);

	void add_child(EntityID p_entity_id);
	void remove_child(EntityID p_entity_id);
};

template <class C>
struct EntityInternal : public EntityBase {
	friend class WorldECS;

	C *owner;
	bool sync_transform = false;

	void _get_property_list(List<PropertyInfo> *p_list) const;
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;

	void _notification(int p_what);

	const OAHashMap<StringName, Variant> &get_components_data() const;

	void add_component(const StringName &p_component_name, const Dictionary &p_values);
	void remove_component(const StringName &p_component_name);
	bool has_component(const StringName &p_component_name) const;

	Dictionary get_component_properties_data(const StringName &p_component) const;

	bool set_component_value(const StringName &p_component_name, const StringName &p_property_name, const Variant &p_value, Space p_space = Space::LOCAL);
	Variant get_component_value(const StringName &p_component_name, const StringName &p_property_name, Space p_space = Space::LOCAL) const;
	bool _get_component_value(const StringName &p_component_name, const StringName &p_property_name, Variant &r_ret, Space p_space = Space::LOCAL) const;

	/// Duplicate this `Entity`.
	uint32_t clone(Object *p_world) const;

	void solve_parenting_with_childs();
	void solve_parenting_with_parent();

	void create_entity();
	EntityID _create_entity(World *p_world) const;
	void destroy_entity();
	void notify_property_list_changed();

	void set_sync_transform(bool p_active) {
		sync_transform = p_active;
	}

	bool get_sync_transform() const {
		return sync_transform;
	}

	// TODO implement this.
	String get_path() const { return owner->get_path(); }
};

/// Check `EntityInternal`, above, for more info.
class Entity3D : public Node3D {
	GDCLASS(Entity3D, Node3D)

	friend class WorldECS;
	EntityInternal<Entity3D> entity;

protected:
	static void _bind_methods();

	void _get_property_list(List<PropertyInfo> *p_list) const {
		entity._get_property_list(p_list);
	}

	bool _set(const StringName &p_name, const Variant &p_value) {
		return entity._set(p_name, p_value);
	}

	bool _get(const StringName &p_name, Variant &r_ret) const {
		return entity._get(p_name, r_ret);
	}

public:
	Entity3D() {
		entity.owner = this;
	}

	void _notification(int p_what) {
		// Handle the transformation
		switch (p_what) {
			case Node::NOTIFICATION_READY:

				set_notify_local_transform(Engine::get_singleton()->is_editor_hint());

				break;
			case Node3D::NOTIFICATION_LOCAL_TRANSFORM_CHANGED:
				if (Engine::get_singleton()->is_editor_hint()) {
					// Handles transform update.
					set_notify_local_transform(false);
					if (has_component("TransformComponent")) {
						set_component_value("TransformComponent", "transform", get_transform());
					} else {
						// This `Entity` doesn't havea TransformComponent, lock
						// the gizmo at center.
						set_transform(Transform());
						set_component_value("TransformComponent", "transform", Transform());
					}
					set_notify_local_transform(true);
				}
				break;
#ifdef TOOLS_ENABLED
			case Node3D::NOTIFICATION_TRANSFORM_CHANGED:
				for (
						OAHashMap<StringName, Ref<ComponentGizmoData>>::Iterator it = entity.gizmo_data.iter();
						it.valid;
						it = entity.gizmo_data.next_iter(it)) {
					if (it.value->is_valid()) {
						(*it.value)->on_position_update(get_global_transform());
					}
				}
				break;
#endif
		}

		entity._notification(p_what);
	}

	uint32_t script_get_entity_id() const { return entity.entity_id; }
	EntityID get_entity_id() const { return entity.entity_id; }

	void add_component(const StringName &p_component_name, const Dictionary &p_values) {
		entity.add_component(p_component_name, p_values);
	}

	void remove_component(const StringName &p_component_name) {
		entity.remove_component(p_component_name);
	}

	bool has_component(const StringName &p_component_name) const {
		return entity.has_component(p_component_name);
	}

	const OAHashMap<StringName, Variant> &get_components_data() const {
		return entity.get_components_data();
	}

	Dictionary get_component_properties_data(const StringName &p_component) const {
		return entity.get_component_properties_data(p_component);
	}

	bool set_component_value(const StringName &p_component_name, const StringName &p_property_name, const Variant &p_value, Space p_space = Space::LOCAL) {
		return entity.set_component_value(p_component_name, p_property_name, p_value, p_space);
	}

	Variant get_component_value(const StringName &p_component_name, const StringName &p_property_name, Space p_space = Space::LOCAL) const {
		return entity.get_component_value(p_component_name, p_property_name, p_space);
	}

	uint32_t clone(Object *p_world) const {
		return entity.clone(p_world);
	}

	void create_entity() {
		entity.create_entity();
	}

	EntityID _create_entity(World *p_world) const {
		return entity._create_entity(p_world);
	}

	void destroy_entity() {
		entity.destroy_entity();
	}

	void update_components_data() {
		entity.notify_property_list_changed();
	}

	EntityInternal<Entity3D> &get_internal_entity() {
		return entity;
	}
	const EntityInternal<Entity3D> &get_internal_entity() const {
		return entity;
	}

	EntityBase &get_internal_entity_base() {
		return entity;
	}
	const EntityBase &get_internal_entity_base() const {
		return entity;
	}

	void set_sync_transform(bool p_active) {
		entity.set_sync_transform(p_active);
		if (p_active) {
			add_to_group("__sync_transform_3d");
		} else {
			remove_from_group("__sync_transform_3d");
		}
	}

	bool get_sync_transform() const {
		return entity.get_sync_transform();
	}
};

/// Check `EntityInternal`, above, for more info.
class Entity2D : public Node2D {
	GDCLASS(Entity2D, Node2D)

	friend class WorldECS;
	EntityInternal<Entity2D> entity;

protected:
	static void _bind_methods();

	void _get_property_list(List<PropertyInfo> *p_list) const {
		entity._get_property_list(p_list);
	}

	bool _set(const StringName &p_name, const Variant &p_value) {
		return entity._set(p_name, p_value);
	}

	bool _get(const StringName &p_name, Variant &r_ret) const {
		return entity._get(p_name, r_ret);
	}

public:
	Entity2D() {
		entity.owner = this;
	}

	void _notification(int p_what) {
		entity._notification(p_what);
	}

	uint32_t script_get_entity_id() const { return entity.entity_id; }
	EntityID get_entity_id() const { return entity.entity_id; }

	void add_component(const StringName &p_component_name, const Dictionary &p_values) {
		entity.add_component(p_component_name, p_values);
	}

	void remove_component(const StringName &p_component_name) {
		entity.remove_component(p_component_name);
	}

	const OAHashMap<StringName, Variant> &get_components_data() const {
		return entity.get_components_data();
	}

	Dictionary get_component_properties_data(const StringName &p_component) const {
		return entity.get_component_properties_data(p_component);
	}

	bool has_component(const StringName &p_component_name) const {
		return entity.has_component(p_component_name);
	}

	bool set_component_value(const StringName &p_component_name, const StringName &p_property_name, const Variant &p_value, Space p_space = Space::LOCAL) {
		return entity.set_component_value(p_component_name, p_property_name, p_value, p_space);
	}

	Variant get_component_value(const StringName &p_component_name, const StringName &p_property_name, Space p_space = Space::LOCAL) const {
		return entity.get_component_value(p_component_name, p_property_name, p_space);
	}

	uint32_t clone(Object *p_world) const {
		return entity.clone(p_world);
	}

	void create_entity() {
		entity.create_entity();
	}

	EntityID _create_entity(World *p_world) const {
		return entity._create_entity(p_world);
	}

	void destroy_entity() {
		entity.destroy_entity();
	}

	void update_components_data() {
		entity.notify_property_list_changed();
	}

	EntityInternal<Entity2D> &get_internal_entity() {
		return entity;
	}

	const EntityInternal<Entity2D> &get_internal_entity() const {
		return entity;
	}

	EntityBase &get_internal_entity_base() {
		return entity;
	}

	const EntityBase &get_internal_entity_base() const {
		return entity;
	}

	void update_gizmo() {
		// TODO no update gizmo for 2D?
	}

	void set_transform(const Transform2D &p_value) {}

	void set_sync_transform(bool p_active) {
		entity.set_sync_transform(p_active);
		if (p_active) {
			add_to_group("__sync_transform_2d");
		} else {
			remove_from_group("__sync_transform_2d");
		}
	}

	bool get_sync_transform() const {
		return entity.get_sync_transform();
	}
};

// TODO this file is full of `get_component_id`. Would be nice cache it.

template <class C>
void EntityInternal<C>::_get_property_list(List<PropertyInfo> *p_list) const {
	for (OAHashMap<StringName, Variant>::Iterator it = components_data.iter(); it.valid; it = components_data.next_iter(it)) {
		p_list->push_back(PropertyInfo(Variant::BOOL, *it.key, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));

		if (ECS::is_component_sharable(ECS::get_component_id(*it.key))) {
			// This is a shared component
			p_list->push_back(PropertyInfo(Variant::OBJECT, String(*it.key) + "/resource", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
		} else {
			// This is a common component.
			const Dictionary &component_properties = it.value->operator Dictionary();

			for (const Variant *key = component_properties.next(); key; key = component_properties.next(key)) {
				const Variant *value = component_properties.getptr(*key);
				p_list->push_back(PropertyInfo(value->get_type(), String(*it.key) + "/" + key->operator String(), PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
				// TODO add here the default value?
			}
		}
	}
}

template <class C>
bool EntityInternal<C>::_set(const StringName &p_name, const Variant &p_value) {
	const Vector<String> names = String(p_name).split("/");
	if (names.size() == 1) {
		// Here we are adding a new component.
		if (unlikely(p_value.get_type() != Variant::BOOL)) {
			WARN_PRINT("When adding a new component using `_set('component_name', true)`, it's not supposed to pass anything different than a TRUE");
		}
		add_component(p_name, Dictionary());
		return true;
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
		// Here we are retrieving the component name, just set true.
		r_ret = true;
		return true;
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
		case ECS::NOTIFICATION_ECS_WORLD_PRE_UNLOAD:
			// Just reset the `EntityID`.
			entity_id = EntityID();
			break;
		case ECS::NOTIFICATION_ECS_WORLD_READY:
			// This happens always after `READY` and differently from
			// `NOTIFICATION_READY` it's not propagated in reverse: so the
			// parent at this point is always initialized.
#ifdef TOOLS_ENABLED
			// At this point the entity is never created.
			CRASH_COND(entity_id.is_null() == false);
#endif
			create_entity();
			// Solving backward because this call is executed first on the parent
			// later on the childs.
			solve_parenting_with_parent();
			break;
		case Node::NOTIFICATION_READY:
			if (Engine::get_singleton()->is_editor_hint()) {
				owner->update_gizmo();
			} else {
				if (ECS::get_singleton()->is_world_ready()) {
					// There is already an active world, just call `ready`.
					create_entity();
					// Solving forward because this function is executed first
					// on the child, later on the parent, so the child entity ID
					// is known at this point.
					solve_parenting_with_childs();
				}
			}
			break;
		case Node::NOTIFICATION_EXIT_TREE:
			if (Engine::get_singleton()->is_editor_hint() == false) {
				destroy_entity();
			}
			break;
		case Node3D::NOTIFICATION_EXIT_WORLD:
#ifdef TOOLS_ENABLED
			gizmo_data.clear();
#endif
			break;
		case Node3D::NOTIFICATION_VISIBILITY_CHANGED: {
			owner->update_gizmo();
		} break;
	}
}

template <class C>
const OAHashMap<StringName, Variant> &EntityInternal<C>::get_components_data() const {
	return components_data;
}

template <class C>
void EntityInternal<C>::add_component(const StringName &p_component_name, const Dictionary &p_values) {
	if (entity_id.is_null()) {
		// We are on editor.
		if (EditorEcs::component_is_shared(p_component_name)) {
			// This is a shared component.
			components_data.set(p_component_name, Variant());
		} else {
			Variant *properties = components_data.lookup_ptr(p_component_name);
			if (properties == nullptr) {
				// The component doesn't exist yet, add it.
				components_data.set(p_component_name, Dictionary());
				properties = components_data.lookup_ptr(p_component_name);
			}
			// Append properties.
			for (const Variant *key = p_values.next(); key; key = p_values.next(key)) {
				// Make sure that the `Key` is always a StringName.
				StringName k = *key;
				properties->operator Dictionary()[k] = *p_values.getptr(*key);
			}
			owner->update_gizmo();
		}
	} else {
		// At runtime during game.
		const godex::component_id id = ECS::get_component_id(p_component_name);
		ERR_FAIL_COND_MSG(id == UINT32_MAX, "The component " + p_component_name + " doesn't exists.");
		ERR_FAIL_COND_MSG(ECS::get_singleton()->has_active_world() == false, "The world is supposed to be active at this point.");
		ECS::get_singleton()->get_active_world()->add_component(entity_id, id, p_values);
	}
}

template <class C>
void EntityInternal<C>::remove_component(const StringName &p_component_name) {
	if (entity_id.is_null()) {
		components_data.remove(p_component_name);
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
		if (EditorEcs::component_is_shared(p_component_name)) {
			const Variant *val = components_data.lookup_ptr(p_component_name);
			if (val) {
				Ref<SharedComponentResource> shared = *val;
				return shared.is_valid() && shared->is_init() && shared->get_component_name() == p_component_name;
			}
			return false;
		} else {
			return components_data.has(p_component_name);
		}

	} else {
		const godex::component_id id = ECS::get_component_id(p_component_name);
		ERR_FAIL_COND_V_MSG(id == UINT32_MAX, false, "The component " + p_component_name + " doesn't exists.");
		ERR_FAIL_COND_V_MSG(ECS::get_singleton()->has_active_world() == false, false, "The world is supposed to be active at this point.");
		return ECS::get_singleton()->get_active_world()->has_component(entity_id, id);
	}
}

template <class C>
Dictionary EntityInternal<C>::get_component_properties_data(const StringName &p_component) const {
	const Variant *val = components_data.lookup_ptr(p_component);
	ERR_FAIL_COND_V_MSG(val == nullptr, Dictionary(), "This component doesn't exists");
	return val->operator Dictionary().duplicate(true);
}

template <class C>
bool EntityInternal<C>::set_component_value(const StringName &p_component_name, const StringName &p_property_name, const Variant &p_value, Space p_space) {
	if (entity_id.is_null()) {
		// We are on editor or the entity doesn't exist yet.

		if (EditorEcs::component_is_shared(p_component_name)) {
			// This is a shared component.

			Ref<SharedComponentResource> shared = p_value;
			if (shared.is_valid()) {
				// Validate the shared component.
				if (shared->is_init()) {
					ERR_FAIL_COND_V_MSG(shared->get_component_name() != p_component_name, false, "The passed component is of type: " + shared->get_component_name() + " while the expected one is of type: " + p_component_name + ".");
				} else {
					// This component is new and not even init, so do it now.
					shared->init(p_component_name);
				}
				components_data.set(p_component_name, shared);
			} else {
				// Try to set the value inside the shared component instead
				Variant *val = components_data.lookup_ptr(p_component_name);
				if (val) {
					shared = *val;
				}

				if (shared.is_null()) {
					// The shared component is still null, so create it.
					shared.instance();
					set_component_value(p_component_name, "", shared);
				}

				shared->set_property_value(p_property_name, p_value);
			}

		} else {
			// This is a standard component.
			ERR_FAIL_COND_V(components_data.has(p_component_name) == false, false);

			if (components_data.lookup_ptr(p_component_name)->get_type() != Variant::DICTIONARY) {
				components_data.set(p_component_name, Dictionary());
			}
			(components_data.lookup_ptr(p_component_name)->operator Dictionary())[p_property_name] = p_value.duplicate();

			// Hack to propagate `Node3D` transform change.
			if (p_component_name == "TransformComponent" && p_property_name == "transform") {
				owner->set_transform(p_value);
			}
		}

		owner->update_gizmo();
		print_line("Component " + p_component_name + " property " + p_property_name + " changed to " + p_value);

		return true;
	} else {
		// This entity exist, so set it on the World.

		ERR_FAIL_COND_V_MSG(ECS::get_singleton()->has_active_world() == false, false, "The world is supposed to be active at this point.");
		const godex::component_id id = ECS::get_component_id(p_component_name);
		ERR_FAIL_COND_V_MSG(id == UINT32_MAX, false, "The component " + p_component_name + " doesn't exists.");
		StorageBase *storage = ECS::get_singleton()->get_active_world()->get_storage(id);
		ERR_FAIL_COND_V_MSG(storage == nullptr, false, "The component " + p_component_name + " doesn't have a storage on the active world.");
		void *component = storage->get_ptr(entity_id, p_space);
		ERR_FAIL_COND_V_MSG(component == nullptr, false, "The entity " + itos(entity_id) + " doesn't have a component " + p_component_name);
		return ECS::unsafe_component_set_by_name(id, component, p_property_name, p_value);
	}
}

template <class C>
Variant EntityInternal<C>::get_component_value(const StringName &p_component_name, const StringName &p_property_name, Space p_space) const {
	Variant ret;
	// No need to test if success because the error is already logged.
	_get_component_value(p_component_name, p_property_name, ret, p_space);
	return ret;
}

template <class C>
bool EntityInternal<C>::_get_component_value(const StringName &p_component_name, const StringName &p_property_name, Variant &r_ret, Space p_space) const {
	// This function is always executed in single thread.

	if (entity_id.is_null()) {
		// Executed on editor or when the entity is not loaded.

		const Variant *component_properties = components_data.lookup_ptr(p_component_name);
		ERR_FAIL_COND_V_MSG(component_properties == nullptr, false, "The component " + p_component_name + " doesn't exist on this entity: " + get_path());

		if (EditorEcs::component_is_shared(p_component_name)) {
			// This is a shared component, so return it.
			Ref<SharedComponentResource> shared = *component_properties;
			if (shared.is_valid() && shared->is_init() && shared->get_component_name() == p_component_name) {
				if (p_property_name == StringName("resource")) {
					// The caller want the entire resource, return it.
					r_ret = shared;
					return true;
				}

				const Variant *val = shared->get_component_data().getptr(p_property_name);
				if (val) {
					r_ret = *val;
					return true;
				} else {
					// The component property is not set, so take the default.
				}
			} else {
				// Can't extract the data from the shared component, take the default.
			}
		} else {
			if (component_properties->get_type() == Variant::DICTIONARY) {
				const Variant *value = (component_properties->operator Dictionary()).getptr(p_property_name);
				if (value != nullptr) {
					// Property is stored, just return it.
					r_ret = value->duplicate(true);
					return true;
				}
			}
		}

		// Property was not found, take the default one.
		return EditorEcs::component_get_property_default_value(
				p_component_name,
				p_property_name,
				r_ret);
	} else {
		// This entity exist on a world, check the value on the world storage.

		ERR_FAIL_COND_V_MSG(ECS::get_singleton()->has_active_world() == false, false, "The world is supposed to be active at this point.");
		const godex::component_id id = ECS::get_component_id(p_component_name);
		ERR_FAIL_COND_V_MSG(id == UINT32_MAX, false, "The component " + p_component_name + " doesn't exists.");
		const StorageBase *storage = ECS::get_singleton()->get_active_world()->get_storage(id);
		ERR_FAIL_COND_V_MSG(storage == nullptr, false, "The component " + p_component_name + " doesn't have a storage on the active world.");
		const void *component = storage->get_ptr(entity_id, p_space);
		ERR_FAIL_COND_V_MSG(component == nullptr, false, "The entity " + itos(entity_id) + " doesn't have a component " + p_component_name);
		return ECS::unsafe_component_get_by_name(id, component, p_property_name, r_ret);
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
void EntityInternal<C>::solve_parenting_with_childs() {
	ERR_FAIL_COND_MSG(entity_id.is_null(), "This is a bug, at this point the entity MUST exist.");

	for (int i = 0; i < owner->get_child_count(); i += 1) {
		EntityBase *child = get_node_entity_base(owner->get_child(i));
		if (child) {
			if (child->components_data.has(ECS::get_component_name(Child::get_component_id()))) {
				ERR_CONTINUE_MSG(child->entity_id.is_null(), "The child entity is expected to exist at this point. This is a bug.");
				add_child(child->entity_id);
			}
		}
	}
}

template <class C>
void EntityInternal<C>::solve_parenting_with_parent() {
	ERR_FAIL_COND_MSG(entity_id.is_null(), "This is a bug, at this point the entity MUST exist.");

	// Fetch the childs
	if (components_data.has(ECS::get_component_name(Child::get_component_id()))) {
		// If this is a child of another `Entity` mark this as
		// child of that.
		EntityBase *parent = get_node_entity_base(owner->get_parent());
		if (parent) {
			ERR_FAIL_COND_MSG(parent->entity_id.is_null(), "The parent entity is expected to exist at this point. This is a bug.");
			// It has an `Entity` parent.
			parent->add_child(entity_id);
		}
	}
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

	owner->propagate_notification(ECS::NOTIFICATION_ECS_ENTITY_CREATED);
}

template <class C>
EntityID EntityInternal<C>::_create_entity(World *p_world) const {
	EntityID id;
	if (p_world) {
		id = p_world->create_entity();

		for (OAHashMap<StringName, Variant>::Iterator it = components_data.iter(); it.valid; it = components_data.next_iter(it)) {
			const uint32_t component_id = ECS::get_component_id(*it.key);
			ERR_CONTINUE(component_id == godex::COMPONENT_NONE);

			if (ECS::is_component_sharable(component_id)) {
				Ref<SharedComponentResource> shared = *it.value;
				if (shared.is_valid()) {
					godex::SID sid = shared->get_sid(p_world);
					p_world->add_shared_component(id, component_id, sid);
				}
			} else {
				p_world->add_component(
						id,
						component_id,
						*it.value);
			}
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

	// Fetch the childs
	if (has_component(ECS::get_component_name(Child::get_component_id()))) {
		// If this is a child of another `Entity` remove from parent.
		EntityBase *parent = get_node_entity_base(owner->get_parent());
		if (parent) {
			// It has an `Entity` parent.
			parent->remove_child(entity_id);
		}
	}

	entity_id = EntityID();
}

template <class C>
void EntityInternal<C>::notify_property_list_changed() {
	owner->notify_property_list_changed();
}
