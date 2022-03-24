#include "systems_base.h"

#include "bullet_types_converter.h"
#include "overlap_check.h"
#include "utilities.h"
#include <BulletCollision/BroadphaseCollision/btBroadphaseProxy.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.h>
#include <BulletCollision/NarrowPhaseCollision/btGjkPairDetector.h>
#include <BulletCollision/NarrowPhaseCollision/btPointCollector.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <btBulletDynamicsCommon.h>

void bt_config_body(
		BtPhysicsSpaces *p_spaces,
		Query<
				EntityID,
				Any<Changed<BtRigidBody>,
						Changed<const BtSpaceMarker>>> &p_query) {
	for (auto [entity, body, space_marker] : p_query) {
		if (body == nullptr) {
			// Body not yet assigned to this Entity, skip.
			continue;
		}

		// Reload mass
		if (body->need_mass_reload()) {
			body->reload_mass();
		}

		// Take the space this body should be on.
		const BtSpaceIndex space_index = space_marker != nullptr ? static_cast<BtSpaceIndex>(space_marker->space_index) : BT_SPACE_0;

		// Make sure the body has a shape.
		if (body->get_shape() == nullptr) {
			body->set_shape(&p_spaces->empty_shape);
		}

		// Reload space
		if ((body->need_body_reload() || body->__current_space != space_index)) {
			// This body needs a realod.

			if (body->__current_space != BtSpaceIndex::BT_SPACE_NONE) {
				// Assume the space is the body is currently on is initialized.
				p_spaces->get_space(body->__current_space)->get_dynamics_world()->removeRigidBody(body->get_body());

				// Set the space this area is on.
				body->get_body()->setUserIndex2(BtSpaceIndex::BT_SPACE_NONE);
			}

			// Now let's add it inside the space again.

			// TODO support space initialization when the body want to stay in another space?
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
		}
	}
}

void bt_config_area(
		BtPhysicsSpaces *p_spaces,
		Query<
				EntityID,
				Any<Changed<BtArea>,
						Changed<const BtSpaceMarker>>>
				&p_query) {
	for (auto [entity, area, space_marker] : p_query.space(GLOBAL)) {
		if (area == nullptr) {
			// Body not yet assigned to this Entity, skip.
			continue;
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

			area->reload_body(space_index);
		}
	}
}

bool equal(const Vector3 &p_vec_1, const Vector3 &p_vec_2) {
	for (int i = 0; i < 3; i += 1) {
		if (Math::abs(p_vec_1[i] - p_vec_2[i]) >= 0.001) {
			return false;
		}
	}
	return true;
}

void bt_teleport_bodies(
		BtPhysicsSpaces *p_spaces,
		Storage<BtBox> *p_shape_storage_box,
		Storage<BtSphere> *p_shape_storage_sphere,
		Query<
				EntityID,
				Changed<const TransformComponent>,
				Any<BtRigidBody, BtArea>> &p_changed_transforms_query) {
	// Update the RigidBody transforms.
	for (auto [entity, transform, body, area] : p_changed_transforms_query) {
		bool is_scale_changed = false;
		// Extract the new scale.
		const Vector3 new_scale = transform->basis.get_scale_abs();

		// Convert the Transform to the bullet transform (which need to be unscaled).
		btTransform t;
		G_TO_B(*transform, t);
		UNSCALE_BT_BASIS(t);

		if (body) {
			is_scale_changed = !equal(body->body_scale, new_scale);
			body->body_scale = new_scale;
			body->set_transform(t, /*Notify changed*/ false);

			// Make sure to remove this entity from the moved bodies.
			if (body->get_motion_state()->space) {
				body->get_motion_state()->space->moved_bodies.remove(entity);
			}

		} else {
			is_scale_changed = !equal(area->area_scale, new_scale);
			area->area_scale = new_scale;
			area->get_ghost()->setWorldTransform(t);
		}

		if (is_scale_changed) {
			// The scale changed, make sure to mark the shape as changed, so it's reloaded.
			if (p_shape_storage_box->has(entity)) {
				p_shape_storage_box->notify_changed(entity);
			} else if (p_shape_storage_sphere->has(entity)) {
				p_shape_storage_sphere->notify_changed(entity);
			}
		}
	}
}

void bt_update_rigidbody_transforms(
		BtPhysicsSpaces *p_spaces,
		Query<const BtRigidBody, Create<InterpolatedTransformComponent>> &p_query) {
	// Update the Transforms of RigidBodies moved by the PhysicsEngine.
	for (uint32_t i = 0; i < BtSpaceIndex::BT_SPACE_MAX; i += 1) {
		const BtSpaceIndex w_i = (BtSpaceIndex)i;

		if (p_spaces->get_space(w_i)->get_dispatcher() == nullptr) {
			// This space is disabled.
			continue;
		}

		p_spaces->get_space(w_i)->moved_bodies.for_each([&](EntityID p_entity_id) {
			if (p_query.has(p_entity_id)) {
				auto [body, interpolated_transform] = p_query.space(GLOBAL)[p_entity_id];

				interpolated_transform->previous_linear_velocity = interpolated_transform->current_linear_velocity;
				B_TO_G(body->get_body()->getLinearVelocity(), interpolated_transform->current_linear_velocity);

				interpolated_transform->previous_transform = interpolated_transform->current_transform;
				B_TO_G(body->get_motion_state()->transf, interpolated_transform->current_transform);
				interpolated_transform->current_transform.scale(body->body_scale);
			}
		});

		p_spaces->get_space(w_i)->moved_bodies.clear();
	}
}

template <class StorageType, class ShapeComponentType>
void bt_config_shape(
		StorageType *p_shape_storage,
		ShapeComponentType *p_shape,
		BtRigidBody *p_body,
		BtArea *p_area) {
	const Vector3 &scale = p_body ? p_body->body_scale : p_area->area_scale;

	// Construct the shape.
	btCollisionShape *shape_ptr = nullptr;
	{
		BtRigidShape::ShapeInfo *shape_info = p_shape->get_shape(scale);
		if (shape_info == nullptr) {
			shape_ptr = p_shape_storage->construct_shape(p_shape, scale);
		} else {
			shape_ptr = shape_info->shape_ptr;
		}
	}

	if (p_body) {
		// Make sure the shape is set.
		p_body->set_shape(shape_ptr);
		// Always reload the mass, so to apply any eventual shape change.
		p_body->reload_mass();
#ifdef DEBUG_ENABLED
		CRASH_COND_MSG(p_body->need_body_reload(), "At this point the body is not supposed to need body realoading, since the shape change doesn't trigger the body reaload and the body is already reloaded by the system bt_config_body.");
#endif
	} else if (p_area) {
		// Make sure the shape is set.
		p_area->set_shape(shape_ptr);
	} else {
		// In this case, nothing to do.
	}
}

void bt_config_box_shape(
		BtShapeStorageBox *p_shape_storage,
		Query<
				Changed<BtBox>,
				Maybe<BtRigidBody>,
				Maybe<BtArea>> &p_query) {
	for (auto [shape, body, area] : p_query) {
		bt_config_shape(p_shape_storage, shape, body, area);
	}
}

void bt_config_sphere_shape(
		BtShapeStorageSphere *p_shape_storage,
		Query<
				Changed<BtSphere>,
				Maybe<BtRigidBody>,
				Maybe<BtArea>> &p_query) {
	for (auto [shape, body, area] : p_query) {
		bt_config_shape(p_shape_storage, shape, body, area);
	}
}

void bt_config_capsule_shape(
		BtShapeStorageCapsule *p_shape_storage,
		Query<
				Changed<BtCapsule>,
				Maybe<BtRigidBody>,
				Maybe<BtArea>> &p_query) {
	for (auto [shape, body, area] : p_query) {
		bt_config_shape(p_shape_storage, shape, body, area);
	}
}

void bt_config_cone_shape(
		BtShapeStorageCone *p_shape_storage,
		Query<
				Changed<BtCone>,
				Maybe<BtRigidBody>,
				Maybe<BtArea>> &p_query) {
	for (auto [shape, body, area] : p_query) {
		bt_config_shape(p_shape_storage, shape, body, area);
	}
}

void bt_config_cylinder_shape(
		BtShapeStorageCylinder *p_shape_storage,
		Query<
				Changed<BtCylinder>,
				Maybe<BtRigidBody>,
				Maybe<BtArea>> &p_query) {
	for (auto [shape, body, area] : p_query) {
		bt_config_shape(p_shape_storage, shape, body, area);
	}
}

void bt_config_convex_shape(
		BtShapeStorageConvex *p_shape_storage,
		Query<
				Changed<BtConvex>,
				Maybe<BtRigidBody>,
				Maybe<BtArea>> &p_query) {
	for (auto [shape, body, area] : p_query) {
		bt_config_shape(p_shape_storage, shape, body, area);
	}
}

void bt_config_trimesh_shape(
		BtShapeStorageTrimesh *p_shape_storage,
		Query<
				Changed<BtTrimesh>,
				Maybe<BtRigidBody>,
				Maybe<BtArea>> &p_query) {
	for (auto [shape, body, area] : p_query) {
		bt_config_shape(p_shape_storage, shape, body, area);
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
				btVector3 force;
				G_TO_B(forces[i]->force, force);
				pawn->external_forces += force;
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
				btVector3 impulse;
				G_TO_B(impulses[i]->impulse, impulse);
				pawn->velocity += impulse;
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
		Storage<BtRigidBody> *,
		Storage<BtArea> *,
		Storage<BtBox> *,
		Storage<BtSphere> *,
		Storage<BtCapsule> *,
		Storage<BtCone> *,
		Storage<BtCylinder> *,
		// Storage<BtWorldMargin> *, Not used.
		Storage<BtConvex> *,
		Storage<BtTrimesh> *) {
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
		EventsEmitter<OverlapStart> &p_enter_event_emitter,
		EventsEmitter<OverlapEnd> &p_exit_event_emitter,
		Query<EntityID, BtArea> &p_query) {
	// Advance the counter.
	p_cache->area_check_frame_counter += 1;
	p_cache->area_check_frame_counter %= 100;
	const int frame_id = p_cache->area_check_frame_counter;

	LocalVector<btCollisionObject *> new_overlaps;

	for (auto [entity, area] : p_query) {
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

		for (int i = int(area->overlaps.size()) - 1; i >= 0; i -= 1) {
			if (area->overlaps[i].detect_frame != frame_id) {
				// This object is no more overlapping

				if (area->exit_emitter_name.is_empty() == false) {
					const EntityID other_entity = area->overlaps[i].object->getUserIndex3();
					OverlapEnd e;
					e.area = entity;
					e.other_body = other_entity;
					p_exit_event_emitter.emit(area->exit_emitter_name, e);
				}

				// Remove the object.
				area->overlaps.remove_at_unordered(i);
			}
		}

		if (area->enter_emitter_name.is_empty() == false) {
			for (uint32_t i = 0; i < new_overlaps.size(); i += 1) {
				const EntityID other_entity = new_overlaps[i]->getUserIndex3();
				OverlapStart e;
				e.area = entity;
				e.other_body = other_entity;
				p_enter_event_emitter.emit(area->enter_emitter_name, e);
			}
		}

		new_overlaps.clear();
	}
}
