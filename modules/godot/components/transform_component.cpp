#include "transform_component.h"

void TransformComponent::_bind_methods() {
	ECS_BIND_PROPERTY(TransformComponent, PropertyInfo(Variant::TRANSFORM, "transform"), transform);
}

void TransformComponent::_get_storage_config(Dictionary &r_dictionary) {
	r_dictionary["pre_allocate"] = 1000;
}

TransformComponent::TransformComponent(const Transform &p_transform) :
		transform(p_transform) {
}

void TransformComponent::combine(
		const TransformComponent &p_local,
		const TransformComponent &p_parent_global,
		TransformComponent &r_global) {
	r_global.transform = p_parent_global.transform * p_local.transform;
}

void TransformComponent::combine_inverse(
		const TransformComponent &p_global,
		const TransformComponent &p_parent_global,
		TransformComponent &r_local) {
	r_local.transform = p_parent_global.transform.inverse() * p_global.transform;
}
