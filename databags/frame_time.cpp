#include "frame_time.h"

void FrameTime::_bind_methods() {
	ECS_BIND_PROPERTY(FrameTime, PropertyInfo(Variant::FLOAT, "delta"), delta);
	ECS_BIND_PROPERTY(FrameTime, PropertyInfo(Variant::FLOAT, "physics_delta"), physics_delta);
}

FrameTime::FrameTime() {
}

void FrameTime::set_exit(bool p_exit) {
	exit = p_exit;
}

bool FrameTime::get_exit() const {
	return exit;
}

void FrameTime::set_main_frame_time(const MainFrameTime &p_frame_time) {
	frame_time = p_frame_time;
}

const MainFrameTime &FrameTime::get_main_frame_time() const {
	return frame_time;
}

void FrameTime::set_delta(real_t p_delta) {
	delta = p_delta;
}

real_t FrameTime::get_delta() const {
	return delta;
}

void FrameTime::set_physics_delta(real_t p_delta) {
	physics_delta = p_delta;
}

real_t FrameTime::get_physics_delta() const {
	return physics_delta;
}

real_t FrameTime::get_physics_interpolation_fraction() const {
	return frame_time.interpolation_fraction;
}
