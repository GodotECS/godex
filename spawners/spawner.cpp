
#include "spawner.h"

#include "../ecs.h"

namespace godex {

bool spawner_validate_insert(spawner_id p_spawner, component_id p_component) {
	ERR_FAIL_COND_V_MSG(ECS::get_spawnable_components(p_spawner).find(p_component) == -1, false, "The component " + ECS::get_component_name(p_component) + " can't be handled by the spawner " + ECS::get_spawner_name(p_spawner) + ". You have to specify the spawner using the macro SPAWNER(" + ECS::get_spawner_name(p_spawner) + ") or by implementing the function `func get_spawners(): return ['" + ECS::get_spawner_name(p_spawner) + "']. Check the doc for more info.");
	return true;
}

void spawner_get_components(spawner_id p_spawner, SystemExeInfo &r_info) {
	const LocalVector<godex::component_id> &spawner = ECS::get_spawnable_components(p_spawner);
	for (uint32_t i = 0; i < spawner.size(); i += 1) {
		r_info.mutable_components_storage.insert(spawner[i]);
	}
}
} // namespace godex
