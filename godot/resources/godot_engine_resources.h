#pragma once

#include "../../resources/ecs_resource.h"
#include "main/main_timer_sync.h"

class OS;
class Engine;
class MessageQueue;

class GodotIteratorInfoResource : public godex::Resource {
	RESOURCE(GodotIteratorInfoResource)

	static void _bind_properties();

	// If set to true, the engine will shut down.
	bool exit = false;

	/// Godot calculates the amount of physics frames for this tick, and put
	/// the information here.
	MainFrameTime frame_time;
	float frame_slice;
	float time_scale;

public:
	GodotIteratorInfoResource();

	void set_exit(bool p_exit);
	bool get_exit() const;

	void set_main_frame_time(const MainFrameTime &p_frame_time);
	const MainFrameTime &get_main_frame_time() const;

	void set_frame_slice(float p_frame_slice);
	float get_frame_slice() const;

	void set_time_scale(float p_time_scale);
	float get_time_scale() const;
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
