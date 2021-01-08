#include "godot_engine_databags.h"

#include "core/config/engine.h"
#include "core/object/message_queue.h"
#include "core/os/os.h"

void FrameTime::_bind_properties() {
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

void OsDatabag::_bind_properties() {}

OsDatabag::OsDatabag() {
	os_singleton = OS::get_singleton();
}

OS *OsDatabag::get_os() {
	return os_singleton;
}

const OS *OsDatabag::get_os() const {
	return os_singleton;
}

void EngineDatabag::_bind_properties() {}

EngineDatabag::EngineDatabag() {
	engine_singleton = Engine::get_singleton();
}

Engine *EngineDatabag::get_engine() {
	return engine_singleton;
}

const Engine *EngineDatabag::get_engine() const {
	return engine_singleton;
}

void MessageQueueDatabag::_bind_properties() {}

MessageQueueDatabag::MessageQueueDatabag() {
	message_queue = MessageQueue::get_singleton();
}

MessageQueue *MessageQueueDatabag::get_queue() {
	return message_queue;
}

const MessageQueue *MessageQueueDatabag::get_queue() const {
	return message_queue;
}
