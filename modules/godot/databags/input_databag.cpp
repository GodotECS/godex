#include "input_databag.h"

void InputDatabag::_bind_methods() {
	add_method("get_input_events_size", &InputDatabag::script_get_input_events_size);
	add_method("get_input_event", &InputDatabag::script_get_input_event);

	add_method("get_from_window_input_events_size", &InputDatabag::script_get_from_window_input_events_size);
	add_method("get_from_window_input_event", &InputDatabag::script_get_from_window_input_event);

	add_method("get_with_modifiers_input_events_size", &InputDatabag::script_get_with_modifiers_input_events_size);
	add_method("get_with_modifiers_input_event", &InputDatabag::script_get_with_modifiers_input_event);

	add_method("get_key_input_events_size", &InputDatabag::script_get_key_input_events_size);
	add_method("get_key_input_event", &InputDatabag::script_get_key_input_event);

	add_method("get_mouse_input_events_size", &InputDatabag::script_get_mouse_input_events_size);
	add_method("get_mouse_input_event", &InputDatabag::script_get_mouse_input_event);

	add_method("get_mouse_button_input_events_size", &InputDatabag::script_get_mouse_button_input_events_size);
	add_method("get_mouse_button_input_event", &InputDatabag::script_get_mouse_button_input_event);

	add_method("get_mouse_motion_input_events_size", &InputDatabag::script_get_mouse_motion_input_events_size);
	add_method("get_mouse_motion_input_event", &InputDatabag::script_get_mouse_motion_input_event);

	add_method("get_joypad_motion_input_events_size", &InputDatabag::script_get_joypad_motion_input_events_size);
	add_method("get_joypad_motion_input_event", &InputDatabag::script_get_joypad_motion_input_event);

	add_method("get_joypad_button_input_events_size", &InputDatabag::script_get_joypad_button_input_events_size);
	add_method("get_joypad_button_input_event", &InputDatabag::script_get_joypad_button_input_event);

	add_method("get_screen_touch_input_events_size", &InputDatabag::script_get_screen_touch_input_events_size);
	add_method("get_screen_touch_input_event", &InputDatabag::script_get_screen_touch_input_event);

	add_method("get_screen_drag_input_events_size", &InputDatabag::script_get_screen_drag_input_events_size);
	add_method("get_screen_drag_input_event", &InputDatabag::script_get_screen_drag_input_event);

	add_method("get_action_input_events_size", &InputDatabag::script_get_action_input_events_size);
	add_method("get_action_input_event", &InputDatabag::script_get_action_input_event);

	add_method("get_gesture_input_events_size", &InputDatabag::script_get_gesture_input_events_size);
	add_method("get_gesture_input_event", &InputDatabag::script_get_gesture_input_event);

	add_method("get_magnify_gesture_input_events_size", &InputDatabag::script_get_magnify_gesture_input_events_size);
	add_method("get_magnify_gesture_input_event", &InputDatabag::script_get_magnify_gesture_input_event);

	add_method("get_pan_gesture_input_events_size", &InputDatabag::script_get_pan_gesture_input_events_size);
	add_method("get_pan_gesture_input_event", &InputDatabag::script_get_pan_gesture_input_event);

	add_method("get_midi_input_events_size", &InputDatabag::script_get_midi_input_events_size);
	add_method("get_midi_input_event", &InputDatabag::script_get_midi_input_event);
}

InputDatabag::InputDatabag() {
	input = Input::get_singleton();
}

Input *InputDatabag::get() {
	return input;
}

const Input *InputDatabag::get() const {
	return input;
}

void InputDatabag::clear_input_events() {
	input_events.clear();
	from_window_input_events.clear();
	with_modifiers_input_events.clear();
	key_input_events.clear();
	mouse_input_events.clear();
	mouse_button_input_events.clear();
	mouse_motion_input_events.clear();
	joypad_motion_input_events.clear();
	joypad_button_input_events.clear();
	screen_touch_input_events.clear();
	screen_drag_input_events.clear();
	action_input_events.clear();
	gesture_input_events.clear();
	magnify_gesture_input_events.clear();
	pan_gesture_input_events.clear();
	midi_input_events.clear();
}

void InputDatabag::add_input_event(const Ref<InputEvent> &p_event) {
	input_events.push_back(p_event);

	{
		Ref<InputEventFromWindow> event = p_event;
		if (event.is_valid()) {
			from_window_input_events.push_back(event);
			// Keep going since this is not a final class.
		}
	}

	{
		Ref<InputEventWithModifiers> event = p_event;
		if (event.is_valid()) {
			with_modifiers_input_events.push_back(event);
			// Keep going since this is not a final class.
		}
	}

	{
		Ref<InputEventKey> event = p_event;
		if (event.is_valid()) {
			key_input_events.push_back(event);
			return;
		}
	}

	{
		Ref<InputEventMouse> event = p_event;
		if (event.is_valid()) {
			mouse_input_events.push_back(event);
			// Keep going since this is not a final class.
		}
	}

	{
		Ref<InputEventMouseButton> event = p_event;
		if (event.is_valid()) {
			mouse_button_input_events.push_back(event);
			return;
		}
	}

	{
		Ref<InputEventMouseMotion> event = p_event;
		if (event.is_valid()) {
			mouse_motion_input_events.push_back(event);
			return;
		}
	}

	{
		Ref<InputEventJoypadMotion> event = p_event;
		if (event.is_valid()) {
			joypad_motion_input_events.push_back(event);
			return;
		}
	}

	{
		Ref<InputEventJoypadButton> event = p_event;
		if (event.is_valid()) {
			joypad_button_input_events.push_back(event);
			return;
		}
	}

	{
		Ref<InputEventScreenTouch> event = p_event;
		if (event.is_valid()) {
			screen_touch_input_events.push_back(event);
			return;
		}
	}

	{
		Ref<InputEventScreenDrag> event = p_event;
		if (event.is_valid()) {
			screen_drag_input_events.push_back(event);
			return;
		}
	}

	{
		Ref<InputEventAction> event = p_event;
		if (event.is_valid()) {
			action_input_events.push_back(event);
			return;
		}
	}

	{
		Ref<InputEventGesture> event = p_event;
		if (event.is_valid()) {
			gesture_input_events.push_back(event);
			// Keep going since this is not a final class.
		}
	}

	{
		Ref<InputEventMagnifyGesture> event = p_event;
		if (event.is_valid()) {
			magnify_gesture_input_events.push_back(event);
			return;
		}
	}

	{
		Ref<InputEventPanGesture> event = p_event;
		if (event.is_valid()) {
			pan_gesture_input_events.push_back(event);
			return;
		}
	}

	{
		Ref<InputEventMIDI> event = p_event;
		if (event.is_valid()) {
			midi_input_events.push_back(event);
			return;
		}
	}

	ERR_PRINT(String("This input event type is not recognized, please open an issue so we can support it: ") + String(Variant(p_event)));
}
