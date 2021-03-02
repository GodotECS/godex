#pragma once

#include "../../databags/databag.h"

#include <BulletCollision/BroadphaseCollision/btBroadphaseInterface.h>
#include <BulletCollision/BroadphaseCollision/btBroadphaseProxy.h>

/*
class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btConstraintSolver;
class btDiscreteDynamicsWorld;
class btSoftBodyWorldInfo;
class btGhostPairCallback;
class GodotFilterCallback;
*/

struct BtWorld {
	//btBroadphaseInterface broadphase;
	//btDefaultCollisionConfiguration collisionConfiguration;
	//btCollisionDispatcher dispatcher;
	//btConstraintSolver solver;
	//btDiscreteDynamicsWorld dynamicsWorld;
	//btGhostPairCallback ghostPairCallback;
	//GodotFilterCallback godotFilterCallback;
};

class BtWorlds : public godex::Databag {
	DATABAG(BtWorlds)

	LocalVector<BtWorld *> worlds;

public:
	BtWorlds();
	~BtWorlds();
};
