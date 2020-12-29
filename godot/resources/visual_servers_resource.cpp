#include "visual_servers_resource.h"

RenderingServerResource::RenderingServerResource() {
	rs = RenderingServer::get_singleton();
}

const RenderingServer *RenderingServerResource::get_rs() const {
	return rs;
}
RenderingServer *RenderingServerResource::get_rs() {
	return rs;
}

void RenderingScenarioResource::_bind_properties() {
	//add_property(PropertyInfo("scenario"), &RenderingScenarioResource::set_scenario, &RenderingScenarioResource::get_scenario);
}

void RenderingScenarioResource::set_scenario(RID p_scenario_rid) {
	changed = true;
	scenario = p_scenario_rid;
}

RID RenderingScenarioResource::get_scenario() const {
	return scenario;
}

void RenderingScenarioResource::set_changed(bool p_changed) {
	changed = p_changed;
}

bool RenderingScenarioResource::is_changed() const {
	return changed;
}
