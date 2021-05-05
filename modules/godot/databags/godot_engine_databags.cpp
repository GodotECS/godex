#include "godot_engine_databags.h"

#include "core/config/engine.h"
#include "core/object/message_queue.h"
#include "core/os/os.h"

void OsDatabag::_bind_methods() {}

OsDatabag::OsDatabag() {
	os_singleton = OS::get_singleton();
}

OS *OsDatabag::get_os() {
	return os_singleton;
}

const OS *OsDatabag::get_os() const {
	return os_singleton;
}

void EngineDatabag::_bind_methods() {}

EngineDatabag::EngineDatabag() {
	engine_singleton = Engine::get_singleton();
}

Engine *EngineDatabag::get_engine() {
	return engine_singleton;
}

const Engine *EngineDatabag::get_engine() const {
	return engine_singleton;
}

void MessageQueueDatabag::_bind_methods() {}

MessageQueueDatabag::MessageQueueDatabag() {
	message_queue = MessageQueue::get_singleton();
}

MessageQueue *MessageQueueDatabag::get_queue() {
	return message_queue;
}

const MessageQueue *MessageQueueDatabag::get_queue() const {
	return message_queue;
}
