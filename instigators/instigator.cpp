
#include "instigator.h"

#include "../ecs.h"

namespace godex {

bool instigator_validate_insert(instigator_id p_instigator, component_id p_component) {
	ERR_FAIL_COND_V_MSG(ECS::get_instigated_components(p_instigator).find(p_component) == -1, false, "The component " + ECS::get_component_name(p_component) + " is not instigated by " + ECS::get_instigator_name(p_instigator) + ". You have to specify the instigator using the macro INSTIGATORS(" + ECS::get_instigator_name(p_instigator) + ") or by implementing the function `func get_instigators(): return ['" + ECS::get_instigator_name(p_instigator) + "']. Check the doc for more info.");
	return true;
}

void instigator_get_components(instigator_id p_instigator, SystemExeInfo &r_info) {
	const LocalVector<godex::component_id> &instigated = ECS::get_instigated_components(p_instigator);
	for (uint32_t i = 0; i < instigated.size(); i += 1) {
		r_info.mutable_components_storage.insert(instigated[i]);
	}
}
} // namespace godex
