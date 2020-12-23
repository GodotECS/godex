#include "physics_process_system.h"

void physics_2d_process_system() {
	//ERR_FAIL_MSG("TODO implement physics process 2D.");
}

void Physics3DServerResource::_bind_properties() {
}

Physics3DServerResource::Physics3DServerResource() {
}

void physics_3d_process_system(Physics3DServerResource *p_resource_physics) {
	ERR_FAIL_COND_MSG(p_resource_physics == nullptr, "The Physics3DServerResource is not part of this world. Add it to use the physics.");

	// TODO
}
