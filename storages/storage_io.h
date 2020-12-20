/* Author: AndreaCatania */

#ifndef STORAGE_WRITER_H
#define STORAGE_WRITER_H

#include "dense_vector.h"

/// Utility that is possible to use to write to and read from a generic storage
/// pointer.
class StorageIO {
public:
	/// Insert the data for a specific entity into the storage. If the data is
	/// not compatible with the storage, it crashes.
	template <class T>
	static void insert(Storage *p_storage, EntityID p_index, T p_data);
};

template <class T>
void StorageIO::insert(Storage *p_storage, EntityID p_index, T p_data) {
	switch (p_storage->get_type()) {
		case StorageType::DENSE_VECTOR:
#ifdef DEBUG_ENABLED
			ERR_FAIL_COND_MSG(dynamic_cast<DenseVector<T> *>(p_storage) == nullptr, "[FATAL] The data type (" + String(typeid(T).name()) + ") is not compatible with the storage type: (" + p_storage->get_type_name() + ")");
#endif
			static_cast<DenseVector<T> *>(p_storage)->insert(p_index, p_data);
			break;
		default:
			ERR_FAIL_MSG("The current storage is not supported, please implement it.");
	}
}

#endif
