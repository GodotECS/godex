#include "godot_engine_resources.h"

#include "core/config/engine.h"
#include "core/object/message_queue.h"
#include "core/os/os.h"

void GodotIteratorInfoResource::_bind_properties() {
}

GodotIteratorInfoResource::GodotIteratorInfoResource() {
}

void GodotIteratorInfoResource::set_exit(bool p_exit) {
	exit = p_exit;
}

bool GodotIteratorInfoResource::get_exit() const {
	return exit;
}

void GodotIteratorInfoResource::set_main_frame_time(const MainFrameTime &p_frame_time) {
	frame_time = p_frame_time;
}

const MainFrameTime &GodotIteratorInfoResource::get_main_frame_time() const {
	return frame_time;
}

void GodotIteratorInfoResource::set_frame_slice(float p_frame_slice) {
	frame_slice = p_frame_slice;
}

float GodotIteratorInfoResource::get_frame_slice() const {
	return frame_slice;
}

void GodotIteratorInfoResource::set_time_scale(float p_time_scale) {
	time_scale = p_time_scale;
}

float GodotIteratorInfoResource::get_time_scale() const {
	return time_scale;
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
