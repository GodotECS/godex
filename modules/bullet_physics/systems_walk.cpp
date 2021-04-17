#include "systems_base.h"

#include "modules/bullet/bullet_types_converter.h"
#include <BulletCollision/BroadphaseCollision/btBroadphaseProxy.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <btBulletDynamicsCommon.h>

void bt_walk(
		BtPhysicsSpaces *p_spaces,
		Query<BtRigidBody, WalkIntention> &p_query) {
	for (auto [rigid_body, walk] : p_query) {
		ERR_CONTINUE_MSG(rigid_body->get_body_mode() != BtRigidBody::RIGID_MODE_KINEMATIC, "The mode of this body is not KINEMATIK");
	}
}
