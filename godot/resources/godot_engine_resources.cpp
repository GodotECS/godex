#include "godot_engine_resources.h"

#include "core/config/engine.h"
#include "core/object/message_queue.h"
#include "core/os/os.h"

void FrameTimeResource::_bind_properties() {
	ECS_BIND_PROPERTY(FrameTimeResource, PropertyInfo(Variant::FLOAT, "delta"), delta);
	ECS_BIND_PROPERTY(FrameTimeResource, PropertyInfo(Variant::FLOAT, "physics_delta"), physics_delta);
}

FrameTimeResource::FrameTimeResource() {
}

void FrameTimeResource::set_exit(bool p_exit) {
	exit = p_exit;
}

bool FrameTimeResource::get_exit() const {
	return exit;
}

void FrameTimeResource::set_main_frame_time(const MainFrameTime &p_frame_time) {
	frame_time = p_frame_time;
}

const MainFrameTime &FrameTimeResource::get_main_frame_time() const {
	return frame_time;
}

void FrameTimeResource::set_delta(real_t p_delta) {
	delta = p_delta;
}

real_t FrameTimeResource::get_delta() const {
	return delta;
}

void FrameTimeResource::set_physics_delta(real_t p_delta) {
	physics_delta = p_delta;
}

real_t FrameTimeResource::get_physics_delta() const {
	return physics_delta;
}

void OsResource::_bind_properties() {}

OsResource::OsResource() {
	os_singleton = OS::get_singleton();
}

OS *OsResource::get_os() {
	return os_singleton;
}

const OS *OsResource::get_os() const {
	return os_singleton;
}

void EngineResource::_bind_properties() {}

EngineResource::EngineResource() {
	engine_singleton = Engine::get_singleton();
}

Engine *EngineResource::get_engine() {
	return engine_singleton;
}

const Engine *EngineResource::get_engine() const {
	return engine_singleton;
}

void MessageQueueResource::_bind_properties() {}

MessageQueueResource::MessageQueueResource() {
	message_queue = MessageQueue::get_singleton();
}

MessageQueue *MessageQueueResource::get_queue() {
	return message_queue;
}

const MessageQueue *MessageQueueResource::get_queue() const {
	return message_queue;
}
