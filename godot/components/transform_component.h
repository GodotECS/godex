/* Author: AndreaCatania */

#ifndef TRANSFOMR_COMPONENT_H
#define TRANSFOMR_COMPONENT_H

#include "../../components/component.h"

// TODO the changed system, here, is really bad and should
// be handled via storage.
class TransformComponent : public godex::Component {
	COMPONENT(TransformComponent, DenseVector)

	Transform transform;
	bool changed = true;

public:
	TransformComponent();
	TransformComponent(const Transform &p_transform);

	void set_transform(const Transform &p_transform);
	const Transform &get_transform() const;
	/// This trigger the transform changed. If you just want to read this,
	/// take it immutable (`const`).
	Transform &get_transform_mut();

	void set_changed(bool p_changed);
	bool is_changed() const;

protected:
	static void _bind_properties();
};

#endif
