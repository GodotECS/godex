#include "bullet_collision_dispatcher.h"

#include "bt_def_type.h"

GodexBtCollisionDispatcher::GodexBtCollisionDispatcher(btCollisionConfiguration *collisionConfiguration) :
		btCollisionDispatcher(collisionConfiguration) {}

bool GodexBtCollisionDispatcher::needsCollision(const btCollisionObject *body0, const btCollisionObject *body1) {
	if (body0->getUserIndex() == BtBodyType::BODY_TYPE_AREA || body1->getUserIndex() == BtBodyType::BODY_TYPE_AREA) {
		// Avoid area narrow phase
		return false;
	}
	return btCollisionDispatcher::needsCollision(body0, body1);
}

bool GodexBtCollisionDispatcher::needsResponse(const btCollisionObject *body0, const btCollisionObject *body1) {
	if (body0->getUserIndex() == BtBodyType::BODY_TYPE_AREA || body1->getUserIndex() == BtBodyType::BODY_TYPE_AREA) {
		// Avoid area narrow phase
		return false;
	}
	return btCollisionDispatcher::needsResponse(body0, body1);
}
