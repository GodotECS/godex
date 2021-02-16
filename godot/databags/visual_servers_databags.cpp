#include "visual_servers_databags.h"

RenderingServerDatabag::RenderingServerDatabag() {
	rs = RenderingServer::get_singleton();
}

const RenderingServer *RenderingServerDatabag::get_rs() const {
	return rs;
}
RenderingServer *RenderingServerDatabag::get_rs() {
	return rs;
}

void RenderingScenarioDatabag::_bind_methods() {
	ECS_BIND_PROPERTY(RenderingScenarioDatabag, PropertyInfo(Variant::OBJECT, "scenario"), scenario);
}
