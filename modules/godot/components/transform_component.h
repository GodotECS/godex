#pragma once

#include "../../../components/component.h"
#include "../../../storage/hierarchical_storage.h"
#include "core/math/transform_3d.h"

class TransformComponent : public Transform3D {
	COMPONENT(TransformComponent, HierarchicalStorage)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_dictionary);

	TransformComponent(const Transform3D &p_transform);

	void set_self_script(const Transform3D &p_transf);
	Transform3D get_self_script() const;

	void set_rotation(const Vector3 &p_euler);
	const Vector3 get_rotation() const;

	void set_rotation_deg(const Vector3 &p_euler);
	const Vector3 get_rotation_deg() const;

	void set_scale(const Vector3 &p_scale);
	const Vector3 get_scale() const;

	/// Used by the `HierarchyStorage` to combine the local data with the parent
	/// global data, and obtain this global.
	static void combine(
			const TransformComponent &p_local,
			const TransformComponent &p_parent_global,
			TransformComponent &r_global);

	/// Used by the `HierarchyStorage` to obtain the local data from the current
	/// global and the parent global.
	static void combine_inverse(
			const TransformComponent &p_global,
			const TransformComponent &p_parent_global,
			TransformComponent &r_local);
};
