#pragma once

#include "../../../components/component.h"
#include "../../../ecs_types.h"
#include "scene/main/node.h"

class Pipeline;
class World;
class WorldECS;
class Entity3D;

/// The `PipelineECS` is a resource that holds the `Pipeline` object, and the
/// info to build it.
/// The `Pipeline` is the class that holds the systems and dispatch them on the
/// given `WorldECS`.
class PipelineECS : public Resource {
	GDCLASS(PipelineECS, Resource)
	/// This name is used to reference this pipeline.
	StringName pipeline_name;

	Array systems_name;
	// TODO add the physics stage

	// This is just a cache value so to avoid rebuild the pipeline each time
	// it's activated.
	Pipeline *pipeline = nullptr;

protected:
	static void _bind_methods();

public:
	PipelineECS();
	~PipelineECS();

	void set_pipeline_name(StringName p_name);
	StringName get_pipeline_name() const;

	void set_systems_name(Array p_system_names);
	Array get_systems_name() const;

	/// Insert a new system into the world. This `System` is not immediately
	/// added to the world. This function is mainly used by the editor to
	/// compose the world.
	///
	/// @param `p_system_name` Can be a native system name (`TransformSystem`)
	///                        or a script system (`ScriptedSystem.gd`)
	/// @param `p_pos` Sets the system at the given position, pushing the others.
	///                Passing `UINT32_MAX`, allow to push back the system.
	///
	/// If the `p_system_name` is already defined it's position is updated.
	///
	/// Note: Add and Remove a `system` will cause world rebuild, which
	/// should be avoided by using more worlds and activating them when
	/// needed.
	void insert_system(const StringName &p_system_name, uint32_t p_pos = UINT32_MAX);

	void remove_system(const StringName &p_system_name);

	void fetch_used_databags(Set<godex::component_id> &r_databags) const;

	/// Builds the pipeline and returns it. The associated world is used to
	/// fetch the pipeline in case a `SystemDispatcher` is used.
	Pipeline *get_pipeline(WorldECS *p_associated_world);
};

/// The `WorldECS` class holds the `World` information that is where all the
/// component storages and the databag storages are stored.
///
/// The `WorldECS` can have many `PipelineECS`, but only one can be active.
/// Pipeline switch can be done only before the pipeline is dispatched TODO explain better how.
///
/// The game can have many phases:
/// - Game Loading, You want to display a loading screen.
/// - Game Running, You want to perform the gamplay logic.
/// Define one pipeline for each stage allow to perform only the needed job
/// for that particular pahase and save computation.
class WorldECS : public Node {
	GDCLASS(WorldECS, Node)

	bool world_build_in_progress = false;
	World *world = nullptr;
	bool want_to_activate = false;
	bool is_active = false;

	Vector<Ref<PipelineECS>> pipelines;
	/// Stores the system dispatchers for the pipelines of this world.
	///  KEY                   VALUE
	/// {SystemDispatcherName: PipelineName}
	Dictionary system_dispatchers_map;
	StringName active_pipeline;

protected:
	static void _bind_methods();
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

public:
	WorldECS();
	virtual ~WorldECS();

	void _notification(int p_what);

	/// Returns the world only if this is not an active world.
	/// If this is an active world and you need to interact with the world is
	/// possible to do it via the commands object that you can take using:
	/// `ECS::get_singleton()->get_commands()`
	World *get_world() const;

	String get_configuration_warning() const override;

	void set_pipelines(Vector<Ref<PipelineECS>> p_pipelines);
	const Vector<Ref<PipelineECS>> &get_pipelines() const;
	Vector<Ref<PipelineECS>> &get_pipelines();

	void add_pipeline(Ref<PipelineECS> p_pipeline);
	void remove_pipeline(Ref<PipelineECS> p_pipeline);

	Ref<PipelineECS> find_pipeline(StringName p_name);
	int find_pipeline_index(StringName p_name) const;

	void set_system_dispatchers_map(Dictionary p_map);
	Dictionary get_system_dispatchers_map() const;

	void set_system_dispatchers_pipeline(const StringName &p_system_name, const StringName &p_pipeline_name);
	StringName get_system_dispatchers_pipeline(const StringName &p_system_name);

	void set_active_pipeline(StringName p_name);
	StringName get_active_pipeline() const;

	void set_storages_config(Dictionary p_config);
	Dictionary get_storages_config() const;

	// ~~ Runtime API ~~
private:
	DataAccessor component_accessor;
	DataAccessor databag_accessor;

public:
	uint32_t create_entity();
	void destroy_entity(uint32_t p_entity_id);

	/// Creates an entity coping the components from the given `Entity`.
	uint32_t create_entity_from_prefab(Object *p_entity);

	void add_component_by_name(uint32_t entity_id, const StringName &p_component_name, const Dictionary &p_data);
	void add_component(uint32_t entity_id, uint32_t p_component_id, const Dictionary &p_data);

	void remove_component_by_name(uint32_t entity_id, const StringName &p_component_name);
	void remove_component(uint32_t entity_id, uint32_t p_component_id);

	/// Returns the component of the entity or null if not assigned.
	/// The returned object lifetime is short, never store it.
	Object *get_entity_component_by_name(uint32_t entity_id, const StringName &p_component_name);
	Object *get_entity_component(uint32_t entity_id, uint32_t p_component_id);

	bool has_entity_component_by_name(uint32_t entity_id, const StringName &p_component_name);
	bool has_entity_component(uint32_t entity_id, uint32_t p_component_id);

	/// Returns the databag or null if not present in the world.
	/// The returned object lifetime is short, never store it.
	Object *get_databag_by_name(const StringName &p_databag_name);
	Object *get_databag(uint32_t p_databag_id);

	void pre_process();
	void post_process();

private:
	void active_world();
	void unactive_world();

	void clear_inputs();
	void on_input(const Ref<InputEvent> &p_ev);

	void sync_3d_transforms();
	void sync_2d_transforms();
};
