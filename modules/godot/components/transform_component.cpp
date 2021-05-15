#include "transform_component.h"

void TransformComponent::_bind_methods() {
	// TODO remove this.
	ECS_BIND_PROPERTY_FUNC(TransformComponent, PropertyInfo(Variant::TRANSFORM, "transform", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR), set_self_script, get_self_script);
	ECS_BIND_PROPERTY(TransformComponent, PropertyInfo(Variant::VECTOR3, "origin", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), origin);
	ECS_BIND_PROPERTY_FUNC(TransformComponent, PropertyInfo(Variant::VECTOR3, "rotation", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), set_rotation, get_rotation);
	ECS_BIND_PROPERTY_FUNC(TransformComponent, PropertyInfo(Variant::VECTOR3, "scale", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), set_scale, get_scale);

	// Usage is set to 0 because we need to expose this only to scripts.
	ECS_BIND_PROPERTY(TransformComponent, PropertyInfo(Variant::VECTOR3, "basis", PROPERTY_HINT_NONE, "", 0), origin);
}

void TransformComponent::_get_storage_config(Dictionary &r_dictionary) {
	r_dictionary["pre_allocate"] = 1000;
}

TransformComponent::TransformComponent(const Transform &p_transform) :
		Transform(p_transform.basis, p_transform.origin) {
}

void TransformComponent::set_self_script(const Transform &p_transf) {
	WARN_PRINT_ONCE("TtransformComponent::set_transform is deprecated. Don't use it please.");
	*this = p_transf;
}

Transform TransformComponent::get_self_script() const {
	WARN_PRINT_ONCE("TtransformComponent::get_transform is deprecated. Don't use it please.");
	return *this;
}

void TransformComponent::set_rotation(const Vector3 &p_euler) {
	basis.set_euler_scale(p_euler, basis.get_scale_local());
}

const Vector3 TransformComponent::get_rotation() const {
	return basis.get_euler();
}

void TransformComponent::set_scale(const Vector3 &p_scale) {
	basis.set_euler_scale(basis.get_euler(), p_scale);
}

const Vector3 TransformComponent::get_scale() const {
	return basis.get_scale();
}

void TransformComponent::combine(
		const TransformComponent &p_local,
		const TransformComponent &p_parent_global,
		TransformComponent &r_global) {
	r_global = p_parent_global * p_local;
}

void TransformComponent::combine_inverse(
		const TransformComponent &p_global,
		const TransformComponent &p_parent_global,
		TransformComponent &r_local) {
	r_local = p_parent_global.inverse() * p_global;
}
