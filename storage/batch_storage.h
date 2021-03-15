#pragma once

#include "../ecs.h"
#include "static_vector.h"
#include "storage.h"

/// Optimized version that allow to store a max size of components consecutivelly,
/// the size must be known at compile time.
template <template <class> class STORAGE, int SIZE, class T>
class BatchStorage : public Storage<T> {
protected:
	STORAGE<StaticVector<T, SIZE>> storage;

public:
	virtual String get_type_name() const override {
		return "FixedSizeBatchStorage<" + String(typeid(T).name()) + ", " + itos(SIZE) + ">";
	}

	virtual void insert(EntityID p_entity, const T &p_data) override {
		if (storage.has(p_entity)) {
			StaticVector<T, SIZE> &v = storage.get(p_entity);
			if (unlikely(v.size() >= SIZE)) {
				// Silently ignore this new data.
			} else {
				v.push_back(p_data);
			}
		} else {
			StaticVector<T, SIZE> v;
			v.push_back(p_data);
			storage.insert(p_entity, v);
		}
		StorageBase::notify_changed(p_entity);
	}

	virtual bool has(EntityID p_entity) const override {
		return storage.has(p_entity);
	}

	virtual T *get(EntityID p_entity, Space p_mode = Space::LOCAL) override {
		StorageBase::notify_changed(p_entity);
		return storage.get(p_entity).ptr();
	}

	virtual const T *get(EntityID p_entity, Space p_mode = Space::LOCAL) const override {
		return storage.get(p_entity).ptr();
	}

	virtual uint32_t get_batch_size(EntityID p_entity) const override {
		return storage.get(p_entity).size();
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

/// The size can be chosen on the fly, but the components are stored in a
/// de-localized memory, which may invalidate cache coherency.
template <template <class> class STORAGE, class T>
class BatchStorage<STORAGE, -1, T> : public Storage<T> {
protected:
	STORAGE<LocalVector<T>> storage;

public:
	virtual String get_type_name() const override {
		return "DynamicSizedBatchStorage<" + String(typeid(T).name()) + ">";
	}

	virtual void insert(EntityID p_entity, const T &p_data) override {
		if (storage.has(p_entity)) {
			storage.get(p_entity).push_back(p_data);
		} else {
			LocalVector<T> s;
			s.resize(1);
			s[0] = p_data;
			storage.insert(p_entity, s);
		}
		StorageBase::notify_changed(p_entity);
	}

	virtual bool has(EntityID p_entity) const override {
		return storage.has(p_entity);
	}

	virtual T *get(EntityID p_entity, Space p_mode = Space::LOCAL) override {
		StorageBase::notify_changed(p_entity);
		LocalVector<T> &data = storage.get(p_entity);
		return data.ptr();
	}

	virtual const T *get(EntityID p_entity, Space p_mode = Space::LOCAL) const override {
		const LocalVector<T> &data = storage.get(p_entity);
		return data.ptr();
	}

	virtual uint32_t get_batch_size(EntityID p_entity) const override {
		return storage.get(p_entity).size();
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
