/* Author: AndreaCatania */

#ifndef TRANSFOMR_COMPONENT_H
#define TRANSFOMR_COMPONENT_H

#include "../../components/component.h"
#include "../../storage/hierarchical_storage.h"

// TODO the CHANGED mechanism is really bad, and MUST be handled via storage.
class TransformComponent : public godex::Component {
	COMPONENT(TransformComponent, HierarchicalStorage)

	Transform transform;
	// TODO remove and use the storage to know if changed.
	bool changed = true;

public:
	TransformComponent() = default;
	TransformComponent(const TransformComponent &) = default;
	TransformComponent(const Transform &p_transform);

	// Transform relative to the parent coordinate system or global if root.
	void set_transform(const Transform &p_transform);
	const Transform &get_transform() const;

	/// This trigger the transform changed. If you just want to read this,
	/// take it immutable (`const`).
	// TODO Rename this to `get_transform`.
	Transform &get_transform_mut();

	void set_changed(bool p_changed); // TODO remove.
	bool is_changed() const; // TODO remove.

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

protected:
	static void _bind_methods();
};

#endif
