#pragma once

#include "../ecs_types.h"

/// Container used to mark the `Entity` as changed.
/// To mark an `Entity` as changed you can use `notify_updated`.
/// To mark an `Entity` as updated (so no more changed) `notify_updated`.
/// It's possible to use the `for_each([](EntityID p_entity){})` to iterate
/// the change list; while iterating it's safe and fast mark the `Entity`
/// as updated (using `notify_updated`).
class EntityList {
	/// Set this to true, disable any kind of modification.
	bool frozen = false;
	/// Sparse vector, used to easily know if an entity changed.
	/// points to the dense_list element.
	LocalVector<uint32_t> entity_to_data;

	/// Used to iterate fast.
	LocalVector<EntityID> dense_list;

	/// Iterator index, used by the mutable for_each` to make sure the update
	/// mechanism is safe even while iterating.
	int64_t iteration_index = -1;

public:
	void freeze();
	void unfreeze();

	void insert(EntityID p_entity);

	void remove(EntityID p_entity);

	bool has(EntityID p_entity) const;

	template <typename F>
	void for_each(F func) {
		for (iteration_index = 0; iteration_index < dense_list.size(); iteration_index += 1) {
			func(dense_list[iteration_index]);
		}
		iteration_index = -1;
	}

	template <typename F>
	void for_each(F func) const {
		for (uint32_t i = 0; i < dense_list.size(); i += 1) {
			func(dense_list[i]);
		}
	}

	bool is_empty() const;

	uint32_t size() const;

	/// Clear the memory, but don't deallocate it so next frame it will run
	/// faster.
	void clear();

	/// Release the memory completely.
	void reset();

	const EntityID *get_entities_ptr() const;
};
