#pragma once

#include "../../components/component.h"

// Tag component to mark `Disabled` things.
class Disabled : public godex::Component {
	COMPONENT(Disabled, DenseVectorStorage)
};
