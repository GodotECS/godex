#include "transform_component.h"

/* Author: AndreaCatania */

TransformComponent::TransformComponent() {}

TransformComponent::TransformComponent(const Transform &p_transform) :
		transform(p_transform) {
}

void TransformComponent::set_transform(const Transform &p_transform) {
	transform = p_transform;
}

const Transform &TransformComponent::get_transform() const {
	return transform;
}

void TransformComponent::_bind_properties() {
	add_property(PropertyInfo(Variant::TRANSFORM, "transform"), &TransformComponent::set_transform, &TransformComponent::get_transform);
}
