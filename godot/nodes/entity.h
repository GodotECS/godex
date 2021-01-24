#pragma once

/* Author: AndreaCatania */

#include "../../components/component.h"
#include "core/templates/local_vector.h"
#include "scene/2d/node_2d.h"
#include "scene/3d/node_3d.h"

class WorldECS;
class World;

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
template <class C>
struct EntityInternal {
	friend class WorldECS;

	C *owner;

	EntityID entity_id;
	Dictionary components_data;

	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;

	void _notification(int p_what);

	void set_components_data(Dictionary p_data);
	const Dictionary &get_components_data() const;

	void add_component(const StringName &p_component_name, const Dictionary &p_values);
	void remove_component(const StringName &p_component_name);
	bool has_component(const StringName &p_component_name) const;

	bool set_component_value(const StringName &p_component_name, const StringName &p_property_name, const Variant &p_value);
	Variant get_component_value(const StringName &p_component_name, const StringName &p_property_name) const;
	bool _get_component_value(const StringName &p_component_name, const StringName &p_property_name, Variant &r_ret) const;

	bool set_component(const StringName &p_component_name, const Variant &d_ret);
	bool _get_component(const StringName &p_component_name, Variant &r_ret) const;

	/// Duplicate this `Entity`.
	uint32_t clone(Object *p_world) const;

	void create_entity();
	EntityID _create_entity(World *p_world) const;
	void destroy_entity();
	void update_components_data();

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
		entity._notification(p_what);
	}

	void set_components_data(Dictionary p_data) { entity.set_components_data(p_data); }
	const Dictionary &get_components_data() const { return entity.get_components_data(); }

	void add_component(const StringName &p_component_name, const Dictionary &p_values) {
		entity.add_component(p_component_name, p_values);
	}

	void remove_component(const StringName &p_component_name) {
		entity.remove_component(p_component_name);
	}

	bool has_component(const StringName &p_component_name) const {
		return entity.has_component(p_component_name);
	}

	bool set_component_value(const StringName &p_component_name, const StringName &p_property_name, const Variant &p_value) {
		return entity.set_component_value(p_component_name, p_property_name, p_value);
	}

	Variant get_component_value(const StringName &p_component_name, const StringName &p_property_name) const {
		return entity.get_component_value(p_component_name, p_property_name);
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
		entity.update_components_data();
	}
};

/// Check `EntityInternal`, above, for more info.
class Entity2D : public Node2D {
	GDCLASS(Entity2D, Node2D)

	friend class WorldECS;
	EntityInternal<Entity2D> entity;

protected:
	static void _bind_methods();

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

	void set_components_data(Dictionary p_data) { entity.set_components_data(p_data); }
	const Dictionary &get_components_data() const { return entity.get_components_data(); }

	void add_component(const StringName &p_component_name, const Dictionary &p_values) {
		entity.add_component(p_component_name, p_values);
	}

	void remove_component(const StringName &p_component_name) {
		entity.remove_component(p_component_name);
	}

	bool has_component(const StringName &p_component_name) const {
		return entity.has_component(p_component_name);
	}

	bool set_component_value(const StringName &p_component_name, const StringName &p_property_name, const Variant &p_value) {
		return entity.set_component_value(p_component_name, p_property_name, p_value);
	}

	Variant get_component_value(const StringName &p_component_name, const StringName &p_property_name) const {
		return entity.get_component_value(p_component_name, p_property_name);
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
		entity.update_components_data();
	}
};

template class EntityInternal<Entity3D>;
template class EntityInternal<Entity2D>;
