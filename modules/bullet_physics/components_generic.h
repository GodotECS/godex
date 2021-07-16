#pragma once

#include "../../components/component.h"
#include "../../storage/dense_vector.h"
#include "../../storage/dense_vector_storage.h"
#include "components_area.h"

struct Force {
	COMPONENT_BATCH(Force, DenseVector, -1)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	Vector3 location;
	Vector3 force;
};

struct Torque {
	COMPONENT_BATCH(Torque, DenseVector, -1)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	Vector3 torque;
};

struct Impulse {
	COMPONENT_BATCH(Impulse, DenseVector, -1)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	Vector3 location;
	Vector3 impulse;
};

struct TorqueImpulse {
	COMPONENT_BATCH(TorqueImpulse, DenseVector, -1)

	static void _bind_methods();
	static void _get_storage_config(Dictionary &r_config);

	Vector3 impulse;
};
