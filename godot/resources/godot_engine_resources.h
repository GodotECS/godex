#pragma once

#include "../../resources/ecs_resource.h"
#include "main/main_timer_sync.h"

class OS;
class Engine;
class MessageQueue;

class FrameTimeResource : public godex::Resource {
	RESOURCE(FrameTimeResource)

	static void _bind_properties();

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
	FrameTimeResource();

	void set_exit(bool p_exit);
	bool get_exit() const;

	void set_main_frame_time(const MainFrameTime &p_frame_time);
	const MainFrameTime &get_main_frame_time() const;

	void set_delta(real_t p_delta);
	real_t get_delta() const;

	void set_physics_delta(real_t p_delta);
	real_t get_physics_delta() const;
};

class OsResource : public godex::Resource {
	RESOURCE(OsResource)

	static void _bind_properties();

	OS *os_singleton;

public:
	OsResource();

	OS *get_os();
	const OS *get_os() const;
};

class EngineResource : public godex::Resource {
	RESOURCE(EngineResource)

	static void _bind_properties();

	Engine *engine_singleton;

public:
	EngineResource();

	Engine *get_engine();
	const Engine *get_engine() const;
};

class MessageQueueResource : public godex::Resource {
	RESOURCE(MessageQueueResource)

	static void _bind_properties();

	MessageQueue *message_queue;

public:
	MessageQueueResource();

	MessageQueue *get_queue();
	const MessageQueue *get_queue() const;
};
