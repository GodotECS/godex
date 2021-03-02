#pragma once

#include "../../../databags/databag.h"
#include "core/input/input.h"

class Input;

class InputDatabag : public godex::Databag {
	DATABAG(InputDatabag)

	Input *input;

public:
	InputDatabag();

	Input *get();
	const Input *get() const;
};
