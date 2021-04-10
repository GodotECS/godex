#include "bullet_result_callbacks.h"

#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>

bool godexBtContactAddedCallback(btManifoldPoint &cp, const btCollisionObjectWrapper *colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper *colObj1Wrap, int partId1, int index1) {
	if (!colObj1Wrap->getCollisionObject()->getCollisionShape()->isCompound()) {
		btAdjustInternalEdgeContacts(cp, colObj1Wrap, colObj0Wrap, partId1, index1);
	}
	return true;
}

bool GodexBtFilterCallback::test_collision_filters(uint32_t body0_collision_layer, uint32_t body0_collision_mask, uint32_t body1_collision_layer, uint32_t body1_collision_mask) {
	return body0_collision_layer & body1_collision_mask || body1_collision_layer & body0_collision_mask;
}

bool GodexBtFilterCallback::needBroadphaseCollision(btBroadphaseProxy *proxy0, btBroadphaseProxy *proxy1) const {
	return GodexBtFilterCallback::test_collision_filters(proxy0->m_collisionFilterGroup, proxy0->m_collisionFilterMask, proxy1->m_collisionFilterGroup, proxy1->m_collisionFilterMask);
}
