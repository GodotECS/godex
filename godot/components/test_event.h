/* Author: AndreaCatania */

#ifndef TEST_EVENT_H
#define TEST_EVENT_H

#include "../../components/component.h"

class TestEvent : public godex::Component {
	COMPONENT(TestEvent, DenseVector)

public:
	TestEvent() {}
};

#endif
