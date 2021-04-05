#pragma once

#include "../storage/storage.h"
#include "../systems/system.h"
#include "../world/world.h"

namespace godex {

#define INSTIGATOR(m_class)                            \
	ECSCLASS(m_class)                                  \
	friend class World;                                \
                                                       \
private:                                               \
	/* Components */                                   \
	static inline uint32_t instigator_id = UINT32_MAX; \
                                                       \
public:                                                \
	static uint32_t get_instigator_id() { return instigator_id; }

template <class... Is>
struct ExtractInstigators {
	void extract(LocalVector<godex::instigator_id> &r_instigators) {}
};

template <class I, class... Is>
struct ExtractInstigators<I, Is...> : public ExtractInstigators<Is...> {
	void extract(LocalVector<godex::instigator_id> &r_instigators) {
		r_instigators.push_back(I::get_instigator_id());
		ExtractInstigators<Is...>::extract(r_instigators);
	}
};

template <class... Is>
LocalVector<godex::instigator_id> internal_get_instigators() {
	LocalVector<godex::instigator_id> instigators;
	ExtractInstigators<Is...> e;
	e.extract(instigators);
	return instigators;
}

#define INSTIGATED(...)                                          \
	static LocalVector<godex::instigator_id> get_instigators() { \
		return godex::internal_get_instigators<__VA_ARGS__>();   \
	}

bool instigator_validate_insert(instigator_id p_instigator, component_id p_component);
void instigator_get_components(instigator_id p_instigator, SystemExeInfo &r_info);
} // namespace godex

/// This class is used by a system to fetch the instigator storages.
/// ```
/// void my_system(Instigator<MyInstigator> &p_instigator){
///		p_instigator->insert<MyComponent>(MyComponent());
/// }
/// ```
template <class I>
class Instigator {
	World *world;

public:
	Instigator(World *p_world) :
			world(p_world) {}

	template <class C>
	void insert(EntityID p_entity, const C &p_data) {
		if (unlikely(godex::instigator_validate_insert(I::get_instigator_id(), C::get_component_id()) == false)) {
			return;
		}
		world->get_storage<C>()->insert(p_entity, p_data);
	}

	template <class C>
	void insert_dynamic(EntityID p_entity, const Dictionary &p_data) {
		insert_dynamic(C::get_component_id(), p_entity, p_data);
	}

	void insert_dynamic(godex::component_id p_component, EntityID p_entity, const Dictionary &p_data) {
		if (unlikely(godex::instigator_validate_insert(I::get_instigator_id(), p_component) == false)) {
			return;
		}
		world->get_storage(p_component)->insert_dynamic(p_entity, p_data);
	}

	static void get_components(SystemExeInfo &r_info) {
		godex::instigator_get_components(I::get_instigator_id(), r_info);
	}
};
