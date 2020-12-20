
/** @author Andrea Catania */

#include "world_commands.h"

const EntityBuilder &WorldCommands::create_entity() {
	return world->create_entity();
}

void WorldCommands::destroy_entity(EntityID p_entity) {
	world->destroy_entity(p_entity);
}

void WorldCommands::add_component(EntityID p_entity, uint32_t p_component_id, const Dictionary &p_data) {
	world->add_component(p_entity, p_component_id, p_data);
}
