#pragma once

#include "../ecs.h"
#include "core/templates/local_vector.h"
#include "storage.h"

template <class T>
class DenseVector {
protected:
	LocalVector<T> data;
	LocalVector<EntityID> data_to_entity;
	// Each position of this vector is an Entity Index.
	LocalVector<uint32_t> entity_to_data;

public:
	void insert(EntityID p_entity, const T &p_data) {
		const uint32_t index = data.size();
		insert_entity(p_entity, index);

		// Store the data
		data.push_back(p_data);
		data_to_entity.push_back(p_entity);
	}

	bool has(EntityID p_entity) const {
		return p_entity < entity_to_data.size() && entity_to_data[p_entity] != UINT32_MAX;
	}

	const T &get(EntityID p_entity) const {
#ifdef DEBUG_ENABLED
		CRASH_COND_MSG(has(p_entity) == false, "This entity doesn't have anything stored into this storage.");
#endif
		return data[entity_to_data[p_entity]];
	}

	T &get(EntityID p_entity) {
#ifdef DEBUG_ENABLED
		CRASH_COND_MSG(has(p_entity) == false, "This entity doesn't have anything stored into this storage.");
#endif
		return data[entity_to_data[p_entity]];
	}

	void remove(EntityID p_entity) {
		ERR_FAIL_COND_MSG(has(p_entity) == false, "This entity doesn't have anything stored into this storage.");

		const uint32_t last = data.size() - 1;

		if (entity_to_data[p_entity] != last) {
			// This entity is the last one, so swap the alst with the current one
			// to remove.

			// Copy the last array element on the data element to remove.
			data[entity_to_data[p_entity]] = data[last];

			// Make sure the entity for the last array element, points to the right slot.
			entity_to_data[data_to_entity[last]] = entity_to_data[p_entity];

			// Now updated the data to entity by simply coping what's in the last.
			data_to_entity[entity_to_data[p_entity]] = data_to_entity[last];
		}

		data.remove_at(last);
		data_to_entity.remove_at(last);
		entity_to_data[p_entity] = UINT32_MAX;
	}

	const LocalVector<EntityID> &get_entities() const {
		return data_to_entity;
	}

	/// Clear the storage.
	void clear() {
		data.clear();
		data_to_entity.clear();
		entity_to_data.clear();
	}

	/// Reset the storage unallocating the memory.
	void reset() {
		data.reset();
		data_to_entity.reset();
		entity_to_data.reset();
	}

	/// Preallocate a given size, avoid useless allocations.
	/// Note, never call this if there are stored data: before call always
	/// `clear`.
	void configure(uint32_t p_reserve) {
		CRASH_COND_MSG(data.size() != 0, "Please call `clear` before `configure`.");

		data.reserve(p_reserve);
		data_to_entity.reserve(p_reserve);
		entity_to_data.reserve(p_reserve);

		entity_to_data.resize(p_reserve);
		for (uint32_t i = 0; i < p_reserve; i += 1) {
			entity_to_data[i] = UINT32_MAX;
		}
	}

protected:
	void insert_entity(EntityID p_entity, uint32_t p_index) {
		if (entity_to_data.size() <= p_entity) {
			const uint32_t start = entity_to_data.size();
			// Resize the vector so to fit this new entity.
			entity_to_data.resize(p_entity + 1);
			for (uint32_t i = start; i < entity_to_data.size(); i += 1) {
				entity_to_data[i] = UINT32_MAX;
			}
		}

		// Store the data-index
		entity_to_data[p_entity] = p_index;
	}
};
