
#include "components_pawn.h"

PawnShape::PawnShape() {
	update_shapes_dimention();
}

void PawnShape::set_pawn_height(real_t p_pawn_height) {
	pawn_height = p_pawn_height;
	update_shapes_dimention();
}

real_t PawnShape::get_pawn_height() const {
	return pawn_height;
}

void PawnShape::set_pawn_radius(real_t p_pawn_radius) {
	pawn_radius = p_pawn_radius;
	update_shapes_dimention();
}

real_t PawnShape::get_pawn_radius() const {
	return pawn_radius;
}

void PawnShape::update_shapes_dimention() {
	main_shape.setImplicitShapeDimensions(btVector3(
			pawn_radius,
			MAX((pawn_height * 0.5) - pawn_radius, 0.0),
			pawn_radius));

	margin_shape.setImplicitShapeDimensions(btVector3(
			pawn_radius + margin,
			MAX((pawn_height * 0.5) - pawn_radius, 0.0) + margin,
			pawn_radius + margin));
}

void BtPawn::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtPawn, PropertyInfo(Variant::INT, "layer", PROPERTY_HINT_LAYERS_3D_PHYSICS), set_layer, get_layer);
	ECS_BIND_PROPERTY_FUNC(BtPawn, PropertyInfo(Variant::INT, "mask", PROPERTY_HINT_LAYERS_3D_PHYSICS), set_mask, get_mask);

	ECS_BIND_PROPERTY(BtPawn, PropertyInfo(Variant::VECTOR3, "velocity"), velocity);
	ECS_BIND_PROPERTY(BtPawn, PropertyInfo(Variant::FLOAT, "step_height"), step_height);
	ECS_BIND_PROPERTY(BtPawn, PropertyInfo(Variant::BASIS, "ground_direction"), ground_direction);
	ECS_BIND_PROPERTY(BtPawn, PropertyInfo(Variant::BOOL, "snap_to_ground"), snap_to_ground);
}

void BtPawn::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 50 Pawns
	/// You can tweak this in editor.
	r_config["page_size"] = 50;
}

BtPawn::BtPawn() {
	body.setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
}

void BtPawn::set_layer(uint32_t p_layer) {
	layer = p_layer;
	need_body_reload = true;
}

uint32_t BtPawn::get_layer() const {
	return layer;
}

void BtPawn::set_mask(uint32_t p_mask) {
	mask = p_mask;
	need_body_reload = true;
}

uint32_t BtPawn::get_mask() const {
	return mask;
}
