/* Author: AndreaCatania */

#pragma once

#include "../ecs.h"
#include "storage.h"

template <template <typename> class X, typename T>
class DynamicBatchStorage : public TypedStorage<T> {
protected:
	X<T> real_storage;

public:
	virtual StorageType get_type() const override;
	virtual String get_type_name() const override;

	virtual void insert(EntityID p_entity, T p_data) override;
	virtual void insert_dynamic(EntityID p_entity, const Dictionary &p_data) override;
	virtual bool has(EntityID p_entity) const override;
	virtual const godex::Component *get_ptr(EntityID p_entity) const;
	virtual godex::Component *get_ptr(EntityID p_entity);
	virtual const T &get(EntityID p_entity) const override;
	virtual T &get(EntityID p_entity) override;
	virtual void remove(EntityID p_entity) override;

protected:
	void insert_entity(EntityID p_entity, uint32_t p_index);
};

template <template <typename> class X, typename T>
StorageType DynamicBatchStorage<X, T>::get_type() const {
	return real_storage.get_type();
}

template <template <typename> class X, typename T>
String DynamicBatchStorage<X, T>::get_type_name() const {
	return real_storage.get_type_name();
}

template <template <typename> class X, typename T>
void DynamicBatchStorage<X, T>::insert_entity(EntityID p_entity, uint32_t p_index) {
	real_storage.insert_entity(p_entity, p_index);
}

template <template <typename> class X, typename T>
void DynamicBatchStorage<X, T>::insert(EntityID p_entity, T p_data) {
	real_storage.insert(p_entity, p_data);
}

template <template <typename> class X, typename T>
void DynamicBatchStorage<X, T>::insert_dynamic(EntityID p_entity, const Dictionary &p_data) {
	real_storage.insert_dynamic(p_entity, p_data);
}

template <template <typename> class X, typename T>
bool DynamicBatchStorage<X, T>::has(EntityID p_entity) const {
	return real_storage.has(p_entity);
}

template <template <typename> class X, typename T>
const godex::Component *DynamicBatchStorage<X, T>::get_ptr(EntityID p_entity) const {
	return real_storage.get_ptr(p_entity);
}

template <template <typename> class X, typename T>
godex::Component *DynamicBatchStorage<X, T>::get_ptr(EntityID p_entity) {
	return real_storage.get_ptr(p_entity);
}

template <template <typename> class X, typename T>
const T &DynamicBatchStorage<X, T>::get(EntityID p_entity) const {
	return real_storage.get(p_entity);
}

template <template <typename> class X, typename T>
T &DynamicBatchStorage<X, T>::get(EntityID p_entity) {
	return real_storage.get(p_entity);
}

template <template <typename> class X, typename T>
void DynamicBatchStorage<X, T>::remove(EntityID p_entity) {
	return real_storage.remove(p_entity);
}
