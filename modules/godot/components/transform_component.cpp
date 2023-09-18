#include "transform_component.h"

#include "core/math/math_funcs.h"

void TransformComponent::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(TransformComponent, PropertyInfo(Variant::TRANSFORM3D, "transform", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), set_self_script, get_self_script);
	ECS_BIND_PROPERTY(TransformComponent, PropertyInfo(Variant::VECTOR3, "origin", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), origin);
	ECS_BIND_PROPERTY_FUNC(TransformComponent, PropertyInfo(Variant::VECTOR3, "rotation_deg", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), set_rotation_deg, get_rotation_deg);
	ECS_BIND_PROPERTY_FUNC(TransformComponent, PropertyInfo(Variant::VECTOR3, "scale", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), set_scale, get_scale);

	// Usage is set to 0 because we need to expose this only to scripts.
	ECS_BIND_PROPERTY(TransformComponent, PropertyInfo(Variant::BASIS, "basis", PROPERTY_HINT_NONE, "", 0), basis);
}

void TransformComponent::_get_storage_config(Dictionary &r_dictionary) {
	r_dictionary["pre_allocate"] = 1000;
}

TransformComponent::TransformComponent(const Transform3D &p_transform) :
		Transform3D(p_transform.basis, p_transform.origin) {
}

void TransformComponent::set_self_script(const Transform3D &p_transf) {
	*this = p_transf;
}

Transform3D TransformComponent::get_self_script() const {
	return *this;
}

void TransformComponent::set_rotation(const Vector3 &p_euler) {
	basis.set_euler_scale(p_euler, basis.get_scale_local());
}

const Vector3 TransformComponent::get_rotation() const {
	return basis.get_euler();
}

void TransformComponent::set_rotation_deg(const Vector3 &p_euler) {
	set_rotation(Vector3(
			Math::deg_to_rad(p_euler[0]),
			Math::deg_to_rad(p_euler[1]),
			Math::deg_to_rad(p_euler[2])));
}

const Vector3 TransformComponent::get_rotation_deg() const {
	const Vector3 r = get_rotation();
	return Vector3(
			Math::rad_to_deg(r[0]),
			Math::rad_to_deg(r[1]),
			Math::rad_to_deg(r[2]));
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
