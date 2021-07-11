#pragma once

#include "../../../iterators/query.h"
#include "../databags/databag_timer.h"
#include "../databags/godot_engine_databags.h"
#include "../databags/scene_tree_databag.h"

/// Updates the `TimerDatabag` timestamp.
void timer_updater_system(TimersDatabag *td, const SceneTreeInfoDatabag *p_sti, const OsDatabag *p_os);

/// Throws events of finished event timers.
void timer_event_launcher_system(TimersDatabag *td);
