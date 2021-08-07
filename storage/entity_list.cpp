#include "entity_list.h"

void EntityList::freeze() {
	frozen = true;
}

void EntityList::unfreeze() {
	frozen = false;
}

void EntityList::insert(EntityID p_entity) {
	if (frozen) {
		return;
	}

	if (entity_to_data.size() <= p_entity) {
		const uint32_t initial_size = entity_to_data.size();
		entity_to_data.resize(p_entity + 1);
		for (uint32_t i = initial_size; i < entity_to_data.size(); i += 1) {
			entity_to_data[i] = UINT32_MAX;
		}
	}
	if (entity_to_data[p_entity] == UINT32_MAX) {
		// This entity was not yet notified.
		entity_to_data[p_entity] = dense_list.size();
		dense_list.push_back(p_entity);
	}
}

void EntityList::remove(EntityID p_entity) {
	if (frozen) {
		return;
	}

	if (entity_to_data.size() <= p_entity) {
		// Was not changed, Nothing to do.
		return;
	}

	const uint32_t index = entity_to_data[p_entity];

	if (iteration_index >= index) {
		// The current iteration_index is bigger than the index
		// to remove: meaning that we already iterated that.
		// Basing on that, it's possible to perform three copy to
		// kick the index out, in a way that the remainin non
		// processed `EntityID` will be processed.
		// *Note: this mechanism allow to correctly process all the
		//        entities while avoiding copy the entire vector to
		//        keep the vector sort.

		// 1. Copy the current index (already processed) on the index to remove.
		dense_list[index] = dense_list[iteration_index];
		entity_to_data[dense_list[index]] = index;

		// 2. Copy the last element on the current index.
		dense_list[iteration_index] = dense_list[dense_list.size() - 1];
		entity_to_data[dense_list[iteration_index]] = iteration_index;

		// 3. Decrese the current index so to process again this index
		//    since it has a new data now.
		iteration_index -= 1;

		// 4. Just resize the array by -1;
		dense_list.resize(dense_list.size() - 1);

		// 5. Clear the entity_pointer since it was removed.
		entity_to_data[p_entity] = UINT32_MAX;
		return;
	}

	// No iteration in progress or not yet iterated.
	if (index != UINT32_MAX) {
		// Remove the element by replacing it with the last one.
		// Assign the currect entity to remove index to the last one.
		entity_to_data[dense_list[dense_list.size() - 1]] = index;
		entity_to_data[p_entity] = UINT32_MAX;
		dense_list[index] = dense_list[dense_list.size() - 1];
		dense_list.resize(dense_list.size() - 1);

		// This code, make sure to decrease by 1 the iterator index, only
		// if it's iterating, otherwise does nothing.
		iteration_index -= 1;
		iteration_index = MAX(-1, iteration_index);
	}
}

bool EntityList::has(EntityID p_entity) const {
	if (entity_to_data.size() <= p_entity) {
		return false;
	}
	return entity_to_data[p_entity] != UINT32_MAX;
}

bool EntityList::is_empty() const {
	return dense_list.size() == 0;
}

uint32_t EntityList::size() const {
	return dense_list.size();
}

void EntityList::clear() {
	// TODO make sure this is really better than just `clear`. I did in this way
	// because this should prevent extra step on insert: not sure it's better
	// at this point. Measure it please.
	for (uint32_t i = 0; i < entity_to_data.size(); i += 1) {
		entity_to_data[i] = UINT32_MAX;
	}
	dense_list.clear();
	frozen = false;
}

void EntityList::reset() {
	entity_to_data.reset();
	dense_list.reset();
	frozen = false;
}

const EntityID *EntityList::get_entities_ptr() const {
	return dense_list.ptr();
}
