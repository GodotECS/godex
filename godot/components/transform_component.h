#pragma once

/* Author: AndreaCatania */

#include "../../components/component.h"
#include "../../storage/hierarchical_storage.h"

class TransformComponent {
	COMPONENT(TransformComponent, HierarchicalStorage)
	static void _bind_methods();

	Transform transform;

	TransformComponent(const Transform &p_transform);

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
