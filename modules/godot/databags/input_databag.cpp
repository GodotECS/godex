#include "input_databag.h"

InputDatabag::InputDatabag() {
	input = Input::get_singleton();
}

Input *InputDatabag::get() {
	return input;
}

const Input *InputDatabag::get() const {
	return input;
}
