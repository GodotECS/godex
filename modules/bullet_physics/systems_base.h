#pragma once

#include "../../databags/frame_time.h"
#include "../godot/components/interpolated_transform_component.h"
#include "../godot/components/transform_component.h"
#include "components_area.h"
#include "components_generic.h"
#include "components_pawn.h"
#include "components_rigid_body.h"
#include "databag_space.h"
#include "events_generic.h"
#include "shape_base.h"
#include "shape_box.h"
#include "shape_capsule.h"
#include "shape_cone.h"
#include "shape_convex.h"
#include "shape_cylinder.h"
#include "shape_sphere.h"
#include "shape_trimesh.h"

/// Configures the body.
/// This `System` is responsible for the body lifetime.
/// - Build the body.
/// - Assigne the Body to the world.
/// - Re-fresh the body if something changed.
void bt_config_body(
		BtPhysicsSpaces *p_spaces,
		Query<
				EntityID,
				Any<Changed<BtRigidBody>,
						Changed<const BtSpaceMarker>>> &p_query);

/// Configures the Area
/// This `System` is responsible for the area lifetime.
/// - Build the area.
/// - Assigne the area to the world.
/// - Re-fresh the area if something changed.
void bt_config_area(
		BtPhysicsSpaces *p_spaces,
		Query<
				EntityID,
				Any<Changed<BtArea>,
						Changed<const BtSpaceMarker>>> &
				p_query);

void bt_teleport_bodies(
		BtPhysicsSpaces *p_spaces,
		Storage<BtBox> *p_shape_storage_box,
		Storage<BtSphere> *p_shape_storage_sphere,
		Query<
				EntityID,
				Changed<const TransformComponent>,
				Any<BtRigidBody, BtArea>> &p_changed_transforms_query);

void bt_update_rigidbody_transforms(
		BtPhysicsSpaces *p_spaces,
		Query<const BtRigidBody, Create<InterpolatedTransformComponent>> &p_query);

void bt_config_box_shape(
		BtShapeStorageBox *p_shape_storage,
		Query<
				Changed<BtBox>,
				Maybe<BtRigidBody>,
				Maybe<BtArea>> &p_query);

void bt_config_sphere_shape(
		BtShapeStorageSphere *p_shape_storage,
		Query<
				Changed<BtSphere>,
				Maybe<BtRigidBody>,
				Maybe<BtArea>> &p_query);

void bt_config_capsule_shape(
		BtShapeStorageCapsule *p_shape_storage,
		Query<
				Changed<BtCapsule>,
				Maybe<BtRigidBody>,
				Maybe<BtArea>> &p_query);

void bt_config_cone_shape(
		BtShapeStorageCone *p_shape_storage,
		Query<
				Changed<BtCone>,
				Maybe<BtRigidBody>,
				Maybe<BtArea>> &p_query);

void bt_config_cylinder_shape(
		BtShapeStorageCylinder *p_shape_storage,
		Query<
				Changed<BtCylinder>,
				Maybe<BtRigidBody>,
				Maybe<BtArea>> &p_query);

void bt_config_convex_shape(
		BtShapeStorageConvex *p_shape_storage,
		Query<
				Changed<BtConvex>,
				Maybe<BtRigidBody>,
				Maybe<BtArea>> &p_query);

void bt_config_trimesh_shape(
		BtShapeStorageTrimesh *p_shape_storage,
		Query<
				Changed<BtTrimesh>,
				Maybe<BtRigidBody>,
				Maybe<BtArea>> &p_query);

void bt_apply_forces(
		Query<BtRigidBody, Batch<Force>, Maybe<BtPawn>> &p_query_forces,
		Query<BtRigidBody, Batch<Torque>> &p_query_torques,
		Query<BtRigidBody, Batch<Impulse>, Maybe<BtPawn>> &p_query_impulses,
		Query<BtRigidBody, Batch<TorqueImpulse>> &p_query_torque_impulses,
		Storage<Impulse> *p_inpulses,
		Storage<TorqueImpulse> *p_torque_inpulses);

// TODO Shape remove from `Entity`

// TODO Body remove from `Entity`

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
		// Storage<BtWorldMargin> *, // Not used.
		Storage<BtConvex> *,
		Storage<BtTrimesh> *);

/// Perform the Areas overlap check.
void bt_overlap_check(
		const BtPhysicsSpaces *p_spaces,
		BtCache *p_cache,
		EventsEmitter<OverlapStart> &p_enter_event_emitter,
		EventsEmitter<OverlapEnd> &p_exit_event_emitter,
		Query<EntityID, BtArea> &p_query);
