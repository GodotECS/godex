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

void RenderingScenarioDatabag::_bind_properties() {
	ECS_BIND_PROPERTY(RenderingScenarioDatabag, PropertyInfo(Variant::OBJECT, "scenario"), scenario);
}

void RenderingScenarioDatabag::set_scenario(RID p_scenario_rid) {
	changed = true;
	scenario = p_scenario_rid;
}

RID RenderingScenarioDatabag::get_scenario() const {
	return scenario;
}

void RenderingScenarioDatabag::set_changed(bool p_changed) {
	changed = p_changed;
}

bool RenderingScenarioDatabag::is_changed() const {
	return changed;
}
