
/**
	@author AndreaCatania
*/

#include "main/main.h"

#include "core/config/engine.h"
#include "core/object/message_queue.h"
#include "core/os/os.h"
#include "ecs.h"
#include "main/main_timer_sync.h"

// TODO just tests
#include "servers/audio_server.h"
#include "servers/display_server.h"
#include "servers/rendering_server.h"

static MainTimerSync main_timer_sync;

// TODO set fixed fps.
static const int fixed_fps = -1;

bool Main::iteration() {
	// Start iteration.
	iterating++;

	// uint64_t idle_process_ticks = 0; // TODO remove?
	bool exit = false;

	const uint64_t ticks = OS::get_singleton()->get_ticks_usec();
	Engine::get_singleton()->_frame_ticks = ticks;
	main_timer_sync.set_cpu_ticks_usec(ticks);
	main_timer_sync.set_fixed_fps(fixed_fps);

	const int physics_fps = Engine::get_singleton()->get_iterations_per_second();
	const float frame_slice = 1.0 / float(physics_fps);
	const float time_scale = Engine::get_singleton()->get_time_scale();

	MainFrameTime advance = main_timer_sync.advance(frame_slice, physics_fps);
	const double step = advance.idle_step;
	const double scaled_step = step * time_scale;

	const uint64_t ticks_elapsed = ticks - last_ticks;

	last_ticks = ticks;

	frame += ticks_elapsed;

	// TODO put this into the pipeline.
	//uint64_t idle_begin = OS::get_singleton()->get_ticks_usec(); // TODO remove?
	MessageQueue::get_singleton()->flush();
	if (OS::get_singleton()->get_main_loop()->idle(step * time_scale)) {
		exit = true;
	}

	MessageQueue::get_singleton()->flush();
	if (ECS::get_singleton()->dispatch_active_world()) {
		exit = true;
	}

	MessageQueue::get_singleton()->flush();
	RenderingServer::get_singleton()->sync(); //sync if still drawing from previous frames.
	if (DisplayServer::get_singleton()->can_any_window_draw() &&
			RenderingServer::get_singleton()->is_render_loop_enabled()) {
		if ((!force_redraw_requested) && OS::get_singleton()->is_in_low_processor_usage_mode()) {
			if (RenderingServer::get_singleton()->has_changed()) {
				RenderingServer::get_singleton()->draw(true, scaled_step); // flush visual commands
				Engine::get_singleton()->frames_drawn++;
			}
		} else {
			RenderingServer::get_singleton()->draw(true, scaled_step); // flush visual commands
			Engine::get_singleton()->frames_drawn++;
			force_redraw_requested = false;
		}
	}

	for (int i = 0; i < ScriptServer::get_language_count(); i++) {
		ScriptServer::get_language(i)->frame();
	}

	AudioServer::get_singleton()->update();

	// End iteration.
	frames++;
	Engine::get_singleton()->_idle_frames++;

	--iterating;

	return exit;
}
