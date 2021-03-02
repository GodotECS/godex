#pragma once

#include "../../../databags/databag.h"
#include "main/main_timer_sync.h"

class OS;
class Engine;
class MessageQueue;

class FrameTime : public godex::Databag {
	DATABAG(FrameTime)

	static void _bind_methods();

	// If set to true, the engine will shut down.
	bool exit = false;

	/// Godot calculates the amount of physics frames for this tick, and put
	/// the information here.
	MainFrameTime frame_time;

	/// Dynamic frame delta.
	real_t delta = 1.0;

	/// Physics delta.
	real_t physics_delta = 1.0;

public:
	FrameTime();

	void set_exit(bool p_exit);
	bool get_exit() const;

	void set_main_frame_time(const MainFrameTime &p_frame_time);
	const MainFrameTime &get_main_frame_time() const;

	void set_delta(real_t p_delta);
	real_t get_delta() const;

	void set_physics_delta(real_t p_delta);
	real_t get_physics_delta() const;
};

class OsDatabag : public godex::Databag {
	DATABAG(OsDatabag)

	static void _bind_methods();

private:
	OS *os_singleton;

public:
	OsDatabag();

	OS *get_os();
	const OS *get_os() const;
};

class EngineDatabag : public godex::Databag {
	DATABAG(EngineDatabag)

	static void _bind_methods();

	Engine *engine_singleton;

public:
	EngineDatabag();

	Engine *get_engine();
	const Engine *get_engine() const;
};

class MessageQueueDatabag : public godex::Databag {
	DATABAG(MessageQueueDatabag)

	static void _bind_methods();

	MessageQueue *message_queue;

public:
	MessageQueueDatabag();

	MessageQueue *get_queue();
	const MessageQueue *get_queue() const;
};
