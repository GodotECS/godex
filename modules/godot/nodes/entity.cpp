
#include "entity.h"

EntityBase *EntityBase::get_node_entity_base(Node *p_node) {
	Entity3D *entity_3d = Object::cast_to<Entity3D>(p_node);
	if (entity_3d == nullptr) {
		Entity2D *entity_2d = Object::cast_to<Entity2D>(p_node);
		if (entity_2d == nullptr) {
			return nullptr;
		}
		return &entity_2d->get_internal_entity_base();
	}
	return &entity_3d->get_internal_entity_base();
}

void EntityBase::add_child(EntityID p_entity_id) {
	ERR_FAIL_COND_MSG(entity_id.is_null(), "The entity_id is not supposed to be null.");
	ERR_FAIL_COND_MSG(p_entity_id.is_null(), "The passed entity_id is not supposed to be null.");

	ECS::get_singleton()->get_active_world()->add_component<Child>(p_entity_id, Child(entity_id));
}

void EntityBase::remove_child(EntityID p_entity_id) {
	ERR_FAIL_COND_MSG(entity_id.is_null(), "The entity_id is not supposed to be null.");
	ERR_FAIL_COND_MSG(p_entity_id.is_null(), "The passed entity_id is not supposed to be null.");

	ECS::get_singleton()->get_active_world()->remove_component<Child>(p_entity_id);
}

void Entity3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_entity_id"), &Entity3D::script_get_entity_id);

	ClassDB::bind_method(D_METHOD("add_component", "component_name", "values"), &Entity3D::add_component, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("remove_component", "component_name"), &Entity3D::remove_component);
	ClassDB::bind_method(D_METHOD("has_component", "component_name"), &Entity3D::has_component);

	ClassDB::bind_method(D_METHOD("set_component_value", "component", "property", "value", "space"), &Entity3D::set_component_value, DEFVAL(Space::LOCAL));
	ClassDB::bind_method(D_METHOD("get_component_value", "component", "property", "space"), &Entity3D::get_component_value, DEFVAL(Space::LOCAL));

	ClassDB::bind_method(D_METHOD("clone", "world"), &Entity3D::clone);

	ClassDB::bind_method(D_METHOD("set_sync_transform", "active"), &Entity3D::set_sync_transform);
	ClassDB::bind_method(D_METHOD("get_sync_transform"), &Entity3D::get_sync_transform);

	ClassDB::bind_method(D_METHOD("set_reference_by_nodepath", "active"), &Entity3D::set_reference_by_nodepath);
	ClassDB::bind_method(D_METHOD("get_reference_by_nodepath"), &Entity3D::get_reference_by_nodepath);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sync_transform"), "set_sync_transform", "get_sync_transform");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "reference_by_nodepath"), "set_reference_by_nodepath", "get_reference_by_nodepath");
}

void Entity2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_entity_id"), &Entity2D::script_get_entity_id);

	ClassDB::bind_method(D_METHOD("add_component", "component_name", "values"), &Entity2D::add_component, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("remove_component", "component_name"), &Entity2D::remove_component);
	ClassDB::bind_method(D_METHOD("has_component", "component_name"), &Entity2D::has_component);

	ClassDB::bind_method(D_METHOD("set_component_value", "component", "property", "value", "space"), &Entity2D::set_component_value, DEFVAL(Space::LOCAL));
	ClassDB::bind_method(D_METHOD("get_component_value", "component", "property", "space"), &Entity2D::get_component_value, DEFVAL(Space::LOCAL));

	ClassDB::bind_method(D_METHOD("clone", "world"), &Entity2D::clone);

	ClassDB::bind_method(D_METHOD("set_sync_transform", "active"), &Entity2D::set_sync_transform);
	ClassDB::bind_method(D_METHOD("get_sync_transform"), &Entity2D::get_sync_transform);

	ClassDB::bind_method(D_METHOD("set_reference_by_nodepath", "active"), &Entity2D::set_reference_by_nodepath);
	ClassDB::bind_method(D_METHOD("get_reference_by_nodepath"), &Entity2D::get_reference_by_nodepath);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sync_transform"), "set_sync_transform", "get_sync_transform");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "reference_by_nodepath"), "set_reference_by_nodepath", "get_reference_by_nodepath");
}
