/* Author: AndreaCatania */

#ifndef DENSE_VECTOR_H
#define DENSE_VECTOR_H

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

		data.remove(last);
		data_to_entity.remove(last);
		entity_to_data[p_entity] = UINT32_MAX;
	}

	void clear() {
		data.clear();
		data_to_entity.clear();
		entity_to_data.clear();
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

/// Dense vector storage.
/// Has a redirection 2-way table between entities and
/// components (vice versa), allowing to leave no gaps within the data.
/// The the entity indices are stored sparsely.
template <class T>
class DenseVectorStorage : public Storage<T> {
protected:
	DenseVector<T> storage;

public:
	virtual String get_type_name() const override {
		return "DenseVector[" + String(typeid(T).name()) + "]";
	}

	virtual void insert(EntityID p_entity, const T &p_data) override {
		storage.insert(p_entity, p_data);
	}

	virtual bool has(EntityID p_entity) const override {
		return storage.has(p_entity);
	}

	virtual Batch<const std::remove_const_t<T>> get(EntityID p_entity, Space p_mode = Space::LOCAL) const override {
		return &storage.get(p_entity);
	}

	virtual Batch<std::remove_const_t<T>> get(EntityID p_entity, Space p_mode = Space::LOCAL) override {
		return &storage.get(p_entity);
	}

	virtual void remove(EntityID p_entity) override {
		storage.remove(p_entity);
	}

	virtual void clear() override {
		storage.clear();
	}
};

template <class T>
class DynamicDenseVectorStorage : public DenseVectorStorage<T> {
	DynamicComponentInfo *dynamic_componente_info = nullptr;

public:
	DynamicDenseVectorStorage(DynamicComponentInfo *p_info) :
			dynamic_componente_info(p_info) {}

	virtual void insert_dynamic(EntityID p_entity, const Dictionary &p_data) override {
		T insert_data;

		// Initialize with defaults.
		insert_data.__initialize(dynamic_componente_info);

		// Set the custom data if any
		for (const Variant *key = p_data.next(); key; key = p_data.next(key)) {
			insert_data.set(StringName(*key), *p_data.getptr(*key));
		}

		this->insert(p_entity, insert_data);
	}
};

#endif
