#include "physics_world.h"

BtWorlds::BtWorlds() {
	worlds.push_back(memnew(BtWorld));
}

BtWorlds::~BtWorlds() {
	for (uint32_t i = 0; i < worlds.size(); i += 1) {
		memdelete(worlds[i]);
	}
	worlds.clear();
}
