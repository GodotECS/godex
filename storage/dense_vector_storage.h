#pragma once

#include "../ecs.h"
#include "dense_vector.h"
#include "storage.h"

/// Dense vector storage.
/// Has a redirection 2-way table between entities and
/// components (vice versa), allowing to leave no gaps within the data.
/// The the entity indices are stored sparsely.
template <class T>
class DenseVectorStorage : public Storage<T> {
protected:
	DenseVector<T> storage;

public:
	virtual void configure(const Dictionary &p_config) override {
		storage.reset();
		storage.configure(p_config.get("pre_allocate", 500));
	}

	virtual String get_type_name() const override {
		return "DenseVector[" + String(typeid(T).name()) + "]";
	}

	virtual void insert(EntityID p_entity, const T &p_data) override {
		storage.insert(p_entity, p_data);
		StorageBase::notify_changed(p_entity);
	}

	virtual bool has(EntityID p_entity) const override {
		return storage.has(p_entity);
	}

	virtual T *get(EntityID p_entity, Space p_mode = Space::LOCAL) override {
		StorageBase::notify_changed(p_entity);
		return &storage.get(p_entity);
	}

	virtual const T *get(EntityID p_entity, Space p_mode = Space::LOCAL) const override {
		return &storage.get(p_entity);
	}

	virtual void remove(EntityID p_entity) override {
		storage.remove(p_entity);
		// Make sure to remove as changed.
		StorageBase::notify_updated(p_entity);
	}

	virtual void clear() override {
		storage.clear();
		StorageBase::flush_changed();
	}

	virtual EntitiesBuffer get_stored_entities() const {
		return { storage.get_entities().size(), storage.get_entities().ptr() };
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
			T::set_by_name(&insert_data, StringName(*key), *p_data.getptr(*key));
		}

		this->insert(p_entity, insert_data);
	}
};
