#pragma once

#include <btBulletDynamicsCommon.h>

/// This class is required to implement custom collision behaviour in the narrowphase
class GodexBtCollisionDispatcher : public btCollisionDispatcher {
public:
	GodexBtCollisionDispatcher(btCollisionConfiguration *collisionConfiguration);
	virtual bool needsCollision(const btCollisionObject *body0, const btCollisionObject *body1);
	virtual bool needsResponse(const btCollisionObject *body0, const btCollisionObject *body1);
};
