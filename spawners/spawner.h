#pragma once

#include "../storage/storage.h"
#include "../systems/system.h"
#include "../world/world.h"

namespace godex {

#define SPAWNER(m_class)                            \
	ECSCLASS(m_class)                               \
	friend class World;                             \
                                                    \
private:                                            \
	/* Components */                                \
	static inline uint32_t spawner_id = UINT32_MAX; \
                                                    \
public:                                             \
	static uint32_t get_spawner_id() { return spawner_id; }

template <class... Is>
struct ExtractSpawners {
	void extract(LocalVector<godex::spawner_id> &r_spawners) {}
};

template <class I, class... Is>
struct ExtractSpawners<I, Is...> : public ExtractSpawners<Is...> {
	void extract(LocalVector<godex::spawner_id> &r_spawners) {
		r_spawners.push_back(I::get_spawner_id());
		ExtractSpawners<Is...>::extract(r_spawners);
	}
};

template <class... Is>
LocalVector<godex::spawner_id> internal_get_spawners() {
	LocalVector<godex::spawner_id> spawners;
	ExtractSpawners<Is...> e;
	e.extract(spawners);
	return spawners;
}

#define SPAWNERS(...)                                       \
	static LocalVector<godex::spawner_id> get_spawners() {  \
		return godex::internal_get_spawners<__VA_ARGS__>(); \
	}

bool spawner_validate_insert(spawner_id p_spawner, component_id p_component);
void spawner_get_components(spawner_id p_spawner, SystemExeInfo &r_info);
} // namespace godex

/// This class is used by a system to fetch the spawner storages.
/// ```
/// void my_system(Spawner<MySpawner> &p_spawner){
///		p_spawner->insert<MyComponent>(MyComponent());
/// }
/// ```
template <class I>
class Spawner {
	World *world = nullptr;

public:
	void initiate_process(World *p_world) {
		world = p_world;
	}
	void release_world() {
		world = nullptr;
	}

	template <class C>
	bool has(EntityID p_entity) {
		return has(C::get_component_id(), p_entity);
	}

	bool has(godex::component_id p_component, EntityID p_entity) {
		if (unlikely(godex::spawner_validate_insert(I::get_spawner_id(), p_component) == false)) {
			return false;
		}
		return world->get_storage(p_component)->has(p_entity);
	}

	template <class C>
	void insert(EntityID p_entity, const C &p_data) {
		if (unlikely(godex::spawner_validate_insert(I::get_spawner_id(), C::get_component_id()) == false)) {
			return;
		}
		world->get_storage<C>()->insert(p_entity, p_data);
	}

	template <class C>
	void insert_dynamic(EntityID p_entity, const Dictionary &p_data) {
		insert_dynamic(C::get_component_id(), p_entity, p_data);
	}

	void insert_dynamic(godex::component_id p_component, EntityID p_entity, const Dictionary &p_data) {
		if (unlikely(godex::spawner_validate_insert(I::get_spawner_id(), p_component) == false)) {
			return;
		}
		world->get_storage(p_component)->insert_dynamic(p_entity, p_data);
	}

	template <class C>
	void remove(EntityID p_entity) {
		remove(C::get_component_id(), p_entity);
	}

	void remove(godex::component_id p_component, EntityID p_entity) {
		if (unlikely(godex::spawner_validate_insert(I::get_spawner_id(), p_component) == false)) {
			return;
		}
		world->get_storage(p_component)->remove(p_entity);
	}

	static void get_components(SystemExeInfo &r_info) {
		godex::spawner_get_components(I::get_spawner_id(), r_info);
	}
};
