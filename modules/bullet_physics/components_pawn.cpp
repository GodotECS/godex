#include "components_pawn.h"

#include "bullet_types_converter.h"

PawnShape::PawnShape() {
	update_shapes_dimention();
}

PawnShape::PawnShape(real_t p_pawn_height, real_t p_pawn_radius) :
		pawn_height(p_pawn_height), pawn_radius(p_pawn_radius) {
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

void PawnShape::set_margin(real_t p_margin) {
	margin = p_margin;
	update_shapes_dimention();
}

real_t PawnShape::get_margin() const {
	return margin;
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

real_t PawnShape::get_enclosing_radius() const {
	return pawn_height + pawn_radius + margin;
}

void BtPawn::_bind_methods() {
	ECS_BIND_PROPERTY_FUNC(BtPawn, PropertyInfo(Variant::VECTOR3, "velocity"), set_velocity, get_velocity);
	ECS_BIND_PROPERTY(BtPawn, PropertyInfo(Variant::FLOAT, "step_height"), step_height);
	ECS_BIND_PROPERTY(BtPawn, PropertyInfo(Variant::BASIS, "ground_direction"), ground_direction);
	ECS_BIND_PROPERTY(BtPawn, PropertyInfo(Variant::BOOL, "snap_to_ground"), snap_to_ground);
	ECS_BIND_PROPERTY(BtPawn, PropertyInfo(Variant::BOOL, "disabled"), disabled);

	ECS_BIND_PROPERTY_FUNC(BtPawn, PropertyInfo(Variant::FLOAT, "stance0_pawn_height"), stance0_set_pawn_height, stance0_get_pawn_height);
	ECS_BIND_PROPERTY_FUNC(BtPawn, PropertyInfo(Variant::FLOAT, "stance0_pawn_radius"), stance0_set_pawn_radius, stance0_get_pawn_radius);
	ECS_BIND_PROPERTY_FUNC(BtPawn, PropertyInfo(Variant::FLOAT, "stance1_pawn_height"), stance1_set_pawn_height, stance1_get_pawn_height);
	ECS_BIND_PROPERTY_FUNC(BtPawn, PropertyInfo(Variant::FLOAT, "stance1_pawn_radius"), stance1_set_pawn_radius, stance1_get_pawn_radius);

	ECS_BIND_PROPERTY_FUNC(BtPawn, PropertyInfo(Variant::FLOAT, "margin"), set_margin, get_margin);
}

void BtPawn::_get_storage_config(Dictionary &r_config) {
	/// Configure the storage of this component to have pages of 50 Pawns
	/// You can tweak this in editor.
	r_config["page_size"] = 50;
}

BtPawn::BtPawn() {
}

void BtPawn::set_velocity(const Vector3 &p_velocity) {
	G_TO_B(p_velocity, velocity);
}

Vector3 BtPawn::get_velocity() const {
	Vector3 v;
	B_TO_G(velocity, v);
	return v;
}

void BtPawn::stance0_set_pawn_height(real_t p_pawn_height) {
	stances[0].set_pawn_height(p_pawn_height);
	update_y_offsets();
}

real_t BtPawn::stance0_get_pawn_height() const {
	return stances[0].get_pawn_height();
}

void BtPawn::stance0_set_pawn_radius(real_t p_pawn_radius) {
	stances[0].set_pawn_radius(p_pawn_radius);
	update_y_offsets();
}

real_t BtPawn::stance0_get_pawn_radius() const {
	return stances[0].get_pawn_radius();
}

void BtPawn::stance1_set_pawn_height(real_t p_pawn_height) {
	stances[1].set_pawn_height(p_pawn_height);
	update_y_offsets();
}
real_t BtPawn::stance1_get_pawn_height() const {
	return stances[1].get_pawn_height();
}
void BtPawn::stance1_set_pawn_radius(real_t p_pawn_radius) {
	stances[1].set_pawn_radius(p_pawn_radius);
	update_y_offsets();
}
real_t BtPawn::stance1_get_pawn_radius() const {
	return stances[1].get_pawn_radius();
}
void BtPawn::set_margin(real_t p_margin) {
	stances[0].set_margin(p_margin);
	stances[1].set_margin(p_margin);
}

real_t BtPawn::get_margin() const {
	return stances[0].get_margin();
}

void BtPawn::update_y_offsets() {
	const real_t stance0_pawn_height = stances[0].get_pawn_height();
	const real_t stance1_pawn_height = stances[1].get_pawn_height();

	const real_t ground_position =
			MAX(stance0_pawn_height, stance1_pawn_height) / -2.0;

	stances[0].offset[1] = ground_position + (stance0_pawn_height / 2.0);
	stances[1].offset[1] = ground_position + (stance1_pawn_height / 2.0);
}
