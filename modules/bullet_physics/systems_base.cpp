#include "systems_base.h"

#include "modules/bullet/bullet_types_converter.h"
#include "overlap_check.h"
#include <BulletCollision/BroadphaseCollision/btBroadphaseProxy.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.h>
#include <BulletCollision/NarrowPhaseCollision/btGjkPairDetector.h>
#include <BulletCollision/NarrowPhaseCollision/btPointCollector.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <btBulletDynamicsCommon.h>

void bt_body_config(
		BtPhysicsSpaces *p_spaces,
		Query<
				EntityID,
				Any<Changed<BtRigidBody>,
						Changed<BtSpaceMarker>,
						Join<
								Changed<BtBox>,
								Changed<BtSphere>,
								Changed<BtCapsule>,
								Changed<BtCone>,
								Changed<BtCylinder>,
								Changed<BtWorldMargin>,
								Changed<BtConvex>,
								Changed<BtTrimesh>,
								Changed<BtStreamedShape>>>,
				Maybe<TransformComponent>> &p_query) {
	for (auto [entity, body, space_marker, shape_container, transform] : p_query.space(GLOBAL)) {
		if (body == nullptr) {
			// Body not yet assigned to this Entity, skip.
			continue;
		}

		// Config shape.
		{
			BtRigidShape *shape = shape_container.as<BtRigidShape>();
			if (shape) {
				if (shape->get_shape()) {
					body->set_shape(shape->get_shape());
				} else if (shape->fallback_empty()) {
					body->set_shape(&p_spaces->empty_shape);
				} else {
					body->set_shape(nullptr);
				}
			} else {
				body->set_shape(nullptr);
			}
		}

		// Reload mass
		if (body->need_mass_reload() && body->get_shape() != nullptr) {
			body->reload_mass(body->get_shape());
		}

		// Take the space this body should be on.
		const BtSpaceIndex space_index = body->get_shape() == nullptr ? BT_SPACE_NONE : (space_marker != nullptr ? static_cast<BtSpaceIndex>(space_marker->space_index) : BT_SPACE_0);

		// Reload space
		if ((body->need_body_reload() ||
					body->__current_space != space_index) &&
				p_spaces != nullptr) {
			// This body needs a realod.

			if (body->__current_space != BtSpaceIndex::BT_SPACE_NONE) {
				// Assume the space is the body is currently on is initialized.
				p_spaces->get_space(body->__current_space)->get_dynamics_world()->removeRigidBody(body->get_body());

				// Set the space this area is on.
				body->get_body()->setUserIndex2(BtSpaceIndex::BT_SPACE_NONE);
			}

			// Now let's add it inside the space again.

			// TODO support space initialization when the body want to stay in
			// another space?
			if (space_index != BtSpaceIndex::BT_SPACE_NONE) {
				BtSpace *space = p_spaces->get_space(space_index);
				space->get_dynamics_world()->addRigidBody(
						body->get_body(),
						body->get_layer(),
						body->get_mask());

				body->get_body()->setMotionState(body->get_motion_state());
				body->get_motion_state()->entity = entity;
				body->get_motion_state()->space = space;

				// Set the space this area is on.
				body->get_body()->setUserIndex2(space_index);

				// Set the EntityID
				body->get_body()->setUserIndex3(entity);

				// Mark as moved
				space->moved_bodies.insert(entity);
			}

			body->reload_body(space_index);
			body->__current_mode = body->get_body_mode();

			// Set transfrorm
			btTransform t;
			if (transform != nullptr) {
				G_TO_B(*transform, t);
			} else {
				t.setIdentity();
			}
			body->set_transform(t, false);
		}
	}
}

void bt_area_config(
		BtPhysicsSpaces *p_spaces,
		Query<
				EntityID,
				Any<Changed<BtArea>,
						Changed<BtSpaceMarker>,
						Join<
								Changed<BtBox>,
								Changed<BtSphere>,
								Changed<BtCapsule>,
								Changed<BtCone>,
								Changed<BtCylinder>,
								Changed<BtWorldMargin>,
								Changed<BtConvex>,
								Changed<BtTrimesh>>>,
				Maybe<TransformComponent>> &p_query) {
	for (auto [entity, area, space_marker, shape_container, transform] : p_query.space(GLOBAL)) {
		if (area == nullptr) {
			// Body not yet assigned to this Entity, skip.
			continue;
		}

		// Config shape.
		BtRigidShape *shape = shape_container.as<BtRigidShape>();
		if (shape != nullptr) {
			if (area->get_shape() != shape->get_shape()) {
				// Area shape is different (or nullptr) form the shape, assign it.
				area->set_shape(shape->get_shape());
			}
		} else {
			if (area->get_shape() != nullptr) {
				// Area has something, but the shape is null, so unassign it.
				area->set_shape(nullptr);
			}
		}

		// Take the space this area should be on.
		const BtSpaceIndex space_index = area->get_shape() == nullptr ? BT_SPACE_NONE : (space_marker != nullptr ? static_cast<BtSpaceIndex>(space_marker->space_index) : BT_SPACE_0);

		// Reload space
		if ((area->need_body_reload() ||
					area->__current_space != space_index) &&
				p_spaces != nullptr) {
			// This area needs a realod.
			if (area->__current_space != BtSpaceIndex::BT_SPACE_NONE) {
				// Assume the space the area is currently on is initialized.
				p_spaces->get_space(area->__current_space)->get_dynamics_world()->removeCollisionObject(area->get_ghost());

				// Set the space this area is on.
				area->get_ghost()->setUserIndex2(BtSpaceIndex::BT_SPACE_NONE);
			}

			// Now let's add it inside the space again.
			if (space_index != BtSpaceIndex::BT_SPACE_NONE) {
				// TODO support space initialization when the area want to stay in
				// another space?
				BtSpace *space = p_spaces->get_space(space_index);
				space->get_dynamics_world()->addCollisionObject(
						area->get_ghost(),
						area->get_layer(),
						area->get_mask());

				// Set the space this area is on.
				area->get_ghost()->setUserIndex2(space_index);

				// Set the EntityID
				area->get_ghost()->setUserIndex3(entity);

				// Mark as moved.
				space->moved_bodies.insert(entity);
			}

			// Set transfrorm
			btTransform t;
			if (transform != nullptr) {
				G_TO_B(*transform, t);
			} else {
				t.setIdentity();
			}
			area->get_ghost()->setWorldTransform(t);

			area->reload_body(space_index);
		}
	}
}

void bt_apply_forces(
		Query<BtRigidBody, Batch<Force>, Maybe<BtPawn>> &p_query_forces,
		Query<BtRigidBody, Batch<Torque>> &p_query_torques,
		Query<BtRigidBody, Batch<Impulse>, Maybe<BtPawn>> &p_query_impulses,
		Query<BtRigidBody, Batch<TorqueImpulse>> &p_query_torque_impulses,
		Storage<Impulse> *p_inpulses,
		Storage<TorqueImpulse> *p_torque_inpulses) {
	for (auto [body, forces, pawn] : p_query_forces) {
		for (uint32_t i = 0; i < forces.get_size(); i += 1) {
			if (pawn) {
				pawn->external_forces += forces[i]->force;
			} else {
				btVector3 l;
				btVector3 f;
				G_TO_B(forces[i]->location, l);
				G_TO_B(forces[i]->force, f);
				body->get_body()->applyForce(f, l);
			}
		}
	}

	for (auto [body, torques] : p_query_torques) {
		for (uint32_t i = 0; i < torques.get_size(); i += 1) {
			btVector3 t;
			G_TO_B(torques[i]->torque, t);
			body->get_body()->applyTorque(t);
		}
	}

	for (auto [body, impulses, pawn] : p_query_impulses) {
		for (uint32_t i = 0; i < impulses.get_size(); i += 1) {
			if (pawn) {
				// The impulse is add directly to velocity.
				pawn->velocity += impulses[i]->impulse;
			} else {
				btVector3 l;
				btVector3 imp;
				G_TO_B(impulses[i]->location, l);
				G_TO_B(impulses[i]->impulse, imp);
				body->get_body()->applyImpulse(imp, l);
			}
		}
	}

	for (auto [body, torque_impulses] : p_query_torque_impulses) {
		for (uint32_t i = 0; i < torque_impulses.get_size(); i += 1) {
			btVector3 imp;
			G_TO_B(torque_impulses[i]->impulse, imp);
			body->get_body()->applyTorqueImpulse(imp);
		}
	}

	// The impulses are impress just for 1 frame.
	p_inpulses->clear();
	p_torque_inpulses->clear();
}

void bt_spaces_step(
		BtPhysicsSpaces *p_spaces,
		const FrameTime *p_iterator_info,
		// TODO this is not used, though we need it just to be sure they are not
		// touched by anything else.
		Query<BtRigidBody, BtArea, BtBox, BtSphere, BtCapsule, BtCone, BtCylinder, BtWorldMargin, BtConvex, BtTrimesh> &p_query) {
	const real_t physics_delta = p_iterator_info->get_physics_delta();

	// TODO consider to create a system for each space? So it has much more control.
	for (uint32_t i = 0; i < BtSpaceIndex::BT_SPACE_MAX; i += 1) {
		BtSpaceIndex w_i = (BtSpaceIndex)i;

		if (p_spaces->get_space(w_i)->get_dispatcher() == nullptr) {
			// This space is disabled.
			continue;
		}

		// Step bullet physics.
		p_spaces->get_space(w_i)->get_dynamics_world()->stepSimulation(
				physics_delta,
				0,
				0);
	}
}

void bt_overlap_check(
		const BtPhysicsSpaces *p_spaces,
		BtCache *p_cache,
		Spawner<OverlapEventSpawner> &p_spawner,
		Query<EntityID, BtArea> &p_query) {
	// Advance the counter.
	p_cache->area_check_frame_counter += 1;
	p_cache->area_check_frame_counter %= 100;
	const int frame_id = p_cache->area_check_frame_counter;

	LocalVector<btCollisionObject *> new_overlaps;

	for (auto [entity, area_] : p_query) {
		BtArea *area = area_;

		if (unlikely(area->__current_space == BT_SPACE_NONE)) {
			// This Area is not in world, nothing to do.
			continue;
		}

		btAlignedObjectArray<btCollisionObject *> &aabb_overlap =
				area->get_ghost()->getOverlappingPairs();

		uint32_t last_found_overlapped_index = 0;

		// This area moved?
		const bool area_is_move =
				p_spaces->get_space(area->__current_space)->moved_bodies.has(entity);

		const btTransform area_transform = area->get_transform() /* X (TODO area scale) */;

		for (int i = 0; i < aabb_overlap.size(); i += 1) {
			// TODO Check if transform changed, if not this overlap is still valid
			// we don't need to check it.

			// Check if this collider is already overlapping.
			const uint32_t overlap_index = area->find_overlapping_object(
					aabb_overlap[i],
					last_found_overlapped_index);

			// This object moved?
			const bool aabb_overlap_is_moved =
					p_spaces->get_space(static_cast<BtSpaceIndex>(aabb_overlap[i]->getUserIndex2()))
							->moved_bodies.has(aabb_overlap[i]->getUserIndex3());

			bool overlapping = false;

			if (overlap_index == UINT32_MAX || area_is_move || aabb_overlap_is_moved) {
				// Extract the other object transform.
				btTransform aabb_overlap_transform;
				if (aabb_overlap[i]->getInternalType() == btCollisionObject::CO_RIGID_BODY) {
					// This is a rigidbody, extrac the transform from the motion_state
					static_cast<btRigidBody *>(aabb_overlap[i])->getMotionState()->getWorldTransform(aabb_overlap_transform);
				} else {
#ifdef DEBUG_ENABLED
					CRASH_COND_MSG(aabb_overlap[i]->getInternalType() != btCollisionObject::CO_GHOST_OBJECT, "Here we expect an area, if you are adding a new type, make sure to update this System.");
#endif
					// This is an area, extract normally.
					aabb_overlap_transform = static_cast<btGhostObject *>(aabb_overlap[i])->getWorldTransform();
				}

				// aabb_overlap_transform TODO multiply the scale.

				// Collision check is required, do it.
				OverlappingFunc func = OverlapCheck::find_algorithm(
						area->get_shape()->getShapeType(),
						aabb_overlap[i]->getCollisionShape()->getShapeType());
				ERR_CONTINUE_MSG(func == nullptr, "No Overlap check Algorithm for this shape pair. Shape A type `" + itos(area->get_shape()->getShapeType()) + "` Shape B type `" + itos(aabb_overlap[i]->getCollisionShape()->getShapeType()) + "`");

				overlapping = func(
						area->get_shape(),
						area_transform,
						aabb_overlap[i]->getCollisionShape(),
						aabb_overlap_transform);
			} else {
				// This object was overlapping and its transform didn't change,
				// skip the collision check.
				overlapping = true;
			}

			if (overlapping) {
				if (overlap_index == UINT32_MAX) {
					// This is a new overlap
					area->add_new_overlap(
							aabb_overlap[i],
							frame_id,
							last_found_overlapped_index);

					// We can increment this by one, so the next overlap can ignore this.
					last_found_overlapped_index += 1;
					new_overlaps.push_back(aabb_overlap[i]);

				} else {
					// This is a known overlap
					area->mark_still_overlapping(overlap_index, frame_id);
					last_found_overlapped_index += overlap_index;
				}
			}
		}

		const godex::component_id component_to_spawn = area->get_overlap_event_component_id();

		// Parse the Overlap data and replace the keywords:
		//  @entity_id is changed with the Area entity_id
		//  @transform is changed with the Area transform
		Dictionary overlap_data = area->overlap_data.duplicate(true);
		for (const Variant *key = overlap_data.next(); key != nullptr; key = overlap_data.next(key)) {
			Variant *val = overlap_data.getptr(*key);
			if (*val == Variant("@entity_id")) {
				*val = entity;
			} else if (*val == Variant("@transform")) {
				Transform3D t;
				B_TO_G(area->get_transform(), t);
				*val = t;
			}
		}

		for (int i = int(area->overlaps.size()) - 1; i >= 0; i -= 1) {
			if (area->overlaps[i].detect_frame != frame_id) {
				// This object is no more overlapping

				if (component_to_spawn != godex::COMPONENT_NONE) {
					const EntityID other_entity = area->overlaps[i].object->getUserIndex3();
					if (area->overlap_event_mode == BtArea::ADD_COMPONENT_ON_EXIT) {
						p_spawner.insert_dynamic(
								component_to_spawn,
								other_entity,
								overlap_data);
					} else if (area->overlap_event_mode == BtArea::KEEP_COMPONENT_WHILE_OVERLAP) {
						// We have to remove the component
						p_spawner.remove(
								component_to_spawn,
								other_entity);
					}
				}

				// Remove the object.
				area->overlaps.remove_unordered(i);
			}
		}

		if (area->overlap_event_mode == BtArea::ADD_COMPONENT_ON_ENTER ||
				area->overlap_event_mode == BtArea::KEEP_COMPONENT_WHILE_OVERLAP) {
			for (uint32_t i = 0; i < new_overlaps.size(); i += 1) {
				const EntityID other_entity = new_overlaps[i]->getUserIndex3();
				p_spawner.insert_dynamic(
						component_to_spawn,
						other_entity,
						overlap_data);
			}
		}

		new_overlaps.clear();
	}
}

void bt_body_sync(
		BtPhysicsSpaces *p_spaces,
		Query<BtRigidBody, TransformComponent> &p_query) {
	for (uint32_t i = 0; i < BtSpaceIndex::BT_SPACE_MAX; i += 1) {
		const BtSpaceIndex w_i = (BtSpaceIndex)i;

		if (p_spaces->get_space(w_i)->get_dispatcher() == nullptr) {
			// This space is disabled.
			continue;
		}

		p_spaces->get_space(w_i)->moved_bodies.for_each([&](EntityID p_entity_id) {
			if (p_query.has(p_entity_id)) {
				auto [body, transform] = p_query.space(GLOBAL)[p_entity_id];
				B_TO_G(body->get_motion_state()->transf, *transform);
			}
		});

		p_spaces->get_space(w_i)->moved_bodies.clear();
	}
}
