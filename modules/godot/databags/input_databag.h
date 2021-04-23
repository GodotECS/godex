#pragma once

#include "../../../databags/databag.h"
#include "core/input/input.h"

class Input;

/// The `InputDatabag` can be used to fetch the player inputs.
///
/// Via this databag is possible to use the `Input` class, or access the
/// `InputEvent`s.
///
/// The `InputEvent`s are classified by type, to simpligfy the access.
/// For example to take the mouse motion you can simply read the vector
/// `mouse_motion_input_events`.
class InputDatabag : public godex::Databag {
	DATABAG(InputDatabag)

	static void _bind_methods();

	Input *input;

public:
	LocalVector<Ref<InputEvent>> input_events;
	LocalVector<Ref<InputEventFromWindow>> from_window_input_events;
	LocalVector<Ref<InputEventWithModifiers>> with_modifiers_input_events;
	LocalVector<Ref<InputEventKey>> key_input_events;
	LocalVector<Ref<InputEventMouse>> mouse_input_events;
	LocalVector<Ref<InputEventMouseButton>> mouse_button_input_events;
	LocalVector<Ref<InputEventMouseMotion>> mouse_motion_input_events;
	LocalVector<Ref<InputEventJoypadMotion>> joypad_motion_input_events;
	LocalVector<Ref<InputEventJoypadButton>> joypad_button_input_events;
	LocalVector<Ref<InputEventScreenTouch>> screen_touch_input_events;
	LocalVector<Ref<InputEventScreenDrag>> screen_drag_input_events;
	LocalVector<Ref<InputEventAction>> action_input_events;
	LocalVector<Ref<InputEventGesture>> gesture_input_events;
	LocalVector<Ref<InputEventMagnifyGesture>> magnify_gesture_input_events;
	LocalVector<Ref<InputEventPanGesture>> pan_gesture_input_events;
	LocalVector<Ref<InputEventMIDI>> midi_input_events;

	InputDatabag();

	Input *get();
	const Input *get() const;

	/// Called by `WorldECS` each frame.
	void clear_input_events();

	/// Used by `WorldECS` to add a new event.
	void add_input_event(const Ref<InputEvent> &p_event);

	uint32_t script_get_input_events_size() const { return input_events.size(); }
	Ref<InputEvent> script_get_input_event(uint32_t i) const { return input_events[i]; }

	uint32_t script_get_from_window_input_events_size() const { return from_window_input_events.size(); }
	Ref<InputEventFromWindow> script_get_from_window_input_event(uint32_t i) const { return from_window_input_events[i]; }

	uint32_t script_get_with_modifiers_input_events_size() const { return with_modifiers_input_events.size(); }
	Ref<InputEventWithModifiers> script_get_with_modifiers_input_event(uint32_t i) const { return with_modifiers_input_events[i]; }

	uint32_t script_get_key_input_events_size() const { return key_input_events.size(); }
	Ref<InputEventKey> script_get_key_input_event(uint32_t i) const { return key_input_events[i]; }

	uint32_t script_get_mouse_input_events_size() const { return mouse_input_events.size(); }
	Ref<InputEventMouse> script_get_mouse_input_event(uint32_t i) const { return mouse_input_events[i]; }

	uint32_t script_get_mouse_button_input_events_size() const { return mouse_button_input_events.size(); }
	Ref<InputEventMouseButton> script_get_mouse_button_input_event(uint32_t i) const { return mouse_button_input_events[i]; }

	uint32_t script_get_mouse_motion_input_events_size() const { return mouse_motion_input_events.size(); }
	Ref<InputEventMouseMotion> script_get_mouse_motion_input_event(uint32_t i) const { return mouse_motion_input_events[i]; }

	uint32_t script_get_joypad_motion_input_events_size() const { return joypad_motion_input_events.size(); }
	Ref<InputEventJoypadMotion> script_get_joypad_motion_input_event(uint32_t i) const { return joypad_motion_input_events[i]; }

	uint32_t script_get_joypad_button_input_events_size() const { return joypad_button_input_events.size(); }
	Ref<InputEventJoypadButton> script_get_joypad_button_input_event(uint32_t i) const { return joypad_button_input_events[i]; }

	uint32_t script_get_screen_touch_input_events_size() const { return screen_touch_input_events.size(); }
	Ref<InputEventScreenTouch> script_get_screen_touch_input_event(uint32_t i) const { return screen_touch_input_events[i]; }

	uint32_t script_get_screen_drag_input_events_size() const { return screen_drag_input_events.size(); }
	Ref<InputEventScreenDrag> script_get_screen_drag_input_event(uint32_t i) const { return screen_drag_input_events[i]; }

	uint32_t script_get_action_input_events_size() const { return action_input_events.size(); }
	Ref<InputEventAction> script_get_action_input_event(uint32_t i) const { return action_input_events[i]; }

	uint32_t script_get_gesture_input_events_size() const { return gesture_input_events.size(); }
	Ref<InputEventGesture> script_get_gesture_input_event(uint32_t i) const { return gesture_input_events[i]; }

	uint32_t script_get_magnify_gesture_input_events_size() const { return magnify_gesture_input_events.size(); }
	Ref<InputEventMagnifyGesture> script_get_magnify_gesture_input_event(uint32_t i) const { return magnify_gesture_input_events[i]; }

	uint32_t script_get_pan_gesture_input_events_size() const { return pan_gesture_input_events.size(); }
	Ref<InputEventPanGesture> script_get_pan_gesture_input_event(uint32_t i) const { return pan_gesture_input_events[i]; }

	uint32_t script_get_midi_input_events_size() const { return midi_input_events.size(); }
	Ref<InputEventMIDI> script_get_midi_input_event(uint32_t i) const { return midi_input_events[i]; }
};
