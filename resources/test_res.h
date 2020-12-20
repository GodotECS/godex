#include "ecs_resource.h"

//TODO just a test

struct TestResource : public ECSResource {
	RESOURCE(TestResource);

public:
	int a = 10;
};