
#include "components_generic.h"

void Force::_bind_methods() {
	ECS_BIND_PROPERTY(Force, PropertyInfo(Variant::VECTOR3, "location"), location);
	ECS_BIND_PROPERTY(Force, PropertyInfo(Variant::VECTOR3, "force"), force);
}

void Force::_get_storage_config(Dictionary &r_config) {
}

void Torque::_bind_methods() {
	ECS_BIND_PROPERTY(Torque, PropertyInfo(Variant::VECTOR3, "torque"), torque);
}

void Torque::_get_storage_config(Dictionary &r_config) {
}

void Impulse::_bind_methods() {
	ECS_BIND_PROPERTY(Impulse, PropertyInfo(Variant::VECTOR3, "location"), location);
	ECS_BIND_PROPERTY(Impulse, PropertyInfo(Variant::VECTOR3, "impulse"), impulse);
}

void Impulse::_get_storage_config(Dictionary &r_config) {
}

void TorqueImpulse::_bind_methods() {
	ECS_BIND_PROPERTY(TorqueImpulse, PropertyInfo(Variant::VECTOR3, "impulse"), impulse);
}

void TorqueImpulse::_get_storage_config(Dictionary &r_config) {
}
