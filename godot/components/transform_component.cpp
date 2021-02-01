#include "transform_component.h"

/* Author: AndreaCatania */

TransformComponent::TransformComponent() {}

TransformComponent::TransformComponent(const Transform &p_transform) :
		transform(p_transform) {
}

void TransformComponent::set_transform(const Transform &p_transform) {
	transform = p_transform;
}

Transform &TransformComponent::get_transform_mut() {
	// Taken mutably, so assume it's changed.
	changed = true;
	return transform;
}

const Transform &TransformComponent::get_transform() const {
	return transform;
}

void TransformComponent::set_changed(bool p_changed) {
	changed = p_changed;
}
bool TransformComponent::is_changed() const {
	return changed;
}

void TransformComponent::combine(
		const TransformComponent &p_local,
		const TransformComponent &p_parent_global,
		TransformComponent &r_global) {
	r_global.transform = p_parent_global.transform * p_local.transform;
}

void TransformComponent::_bind_methods() {
	ECS_BIND_PROPERTY(TransformComponent, PropertyInfo(Variant::TRANSFORM, "transform"), transform);
}
