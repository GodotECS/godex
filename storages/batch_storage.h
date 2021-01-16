/* Author: AndreaCatania */

#pragma once

#include "../ecs.h"
#include "static_vector.h"
#include "storage.h"

/// Optimized version that allow to store a max size of components consecutivelly,
/// the size must be known at compile time.
template <template <class> class STORAGE, int SIZE, class T>
class BatchStorage : public TypedStorage<T> {
protected:
	// TODO Here we have the size, can we use an old style array instead?
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
	}

	virtual void insert_dynamic(EntityID p_entity, const Dictionary &p_data) override {
		T insert_data;

		// Set the custom data if any.
		for (const Variant *key = p_data.next(); key; key = p_data.next(key)) {
			insert_data.set(StringName(*key), *p_data.getptr(*key));
		}

		insert(p_entity, insert_data);
	}

	virtual bool has(EntityID p_entity) const override {
		return storage.has(p_entity);
	}

	virtual Batch<const godex::Component> get_ptr(EntityID p_entity) const {
		return get(p_entity);
	}

	virtual Batch<godex::Component> get_ptr(EntityID p_entity) {
		return get(p_entity);
	}

	virtual Batch<const T> get(EntityID p_entity) const override {
		const StaticVector<T, SIZE> &data = storage.get(p_entity);
		return Batch(data.ptr(), data.size());
	}

	virtual Batch<T> get(EntityID p_entity) override {
		StaticVector<T, SIZE> &data = storage.get(p_entity);
		return Batch(data.ptr(), data.size());
	}

	virtual void remove(EntityID p_entity) override {
		storage.remove(p_entity);
	}
};

/// The size can be chosen on the fly, but the components are stored in a
/// de-localized memory, which may invalidate cache coherency.
template <template <class> class STORAGE, class T>
class BatchStorage<STORAGE, -1, T> : public TypedStorage<T> {
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
	}

	virtual void insert_dynamic(EntityID p_entity, const Dictionary &p_data) override {
		T insert_data;

		// Set the custom data if any.
		for (const Variant *key = p_data.next(); key; key = p_data.next(key)) {
			insert_data.set(StringName(*key), *p_data.getptr(*key));
		}

		insert(p_entity, insert_data);
	}

	virtual bool has(EntityID p_entity) const override {
		return storage.has(p_entity);
	}

	virtual Batch<const godex::Component> get_ptr(EntityID p_entity) const {
		return get(p_entity);
	}

	virtual Batch<godex::Component> get_ptr(EntityID p_entity) {
		return get(p_entity);
	}

	virtual Batch<const T> get(EntityID p_entity) const override {
		const LocalVector<T> &data = storage.get(p_entity);
		return Batch(data.ptr(), data.size());
	}

	virtual Batch<T> get(EntityID p_entity) override {
		LocalVector<T> &data = storage.get(p_entity);
		return Batch(data.ptr(), data.size());
	}

	virtual void remove(EntityID p_entity) override {
		storage.remove(p_entity);
	}
};
