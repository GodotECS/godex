#pragma once

#include "servers/physics_server_3d.h"

#include <BulletCollision/BroadphaseCollision/btBroadphaseProxy.h>
#include <btBulletDynamicsCommon.h>

/// This callback is injected inside bullet server and allow me to smooth
/// contacts against trimesh
bool godexBtContactAddedCallback(btManifoldPoint &cp, const btCollisionObjectWrapper *colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper *colObj1Wrap, int partId1, int index1);

/// This class is required to implement custom collision behaviour in the
/// broadphase.
struct GodexBtFilterCallback : public btOverlapFilterCallback {
	static bool test_collision_filters(uint32_t body0_collision_layer, uint32_t body0_collision_mask, uint32_t body1_collision_layer, uint32_t body1_collision_mask);

	// return true when pairs need collision
	virtual bool needBroadphaseCollision(btBroadphaseProxy *proxy0, btBroadphaseProxy *proxy1) const;
};
