#pragma once

#include "../thirdparty/typestring.hh"
#include "../world/world.h"

#define EMITTER(str) typestring_is(#str)

/// Utility to emit an ECS event. This can be used by c++ systems:
/// ```
/// void my_emitter_system(EventsEmitter<MyEvent> &p_emitter){
///		p_emitter.emit("EmitterName1", MyEvent());
///		p_emitter.emit("EmitterName2", MyEvent());
/// }
/// ```
template <class E>
class EventsEmitter {
	EventStorage<E> *storage = nullptr;

public:
	void initiate_process(World *p_world) {
		storage = p_world->get_events_storage<E>();
		// Flush the old events.
		storage->flush_events();
	}

	void release_world() {
		storage = nullptr;
	}

	void emit(const String &p_emitter_name, const E &p_event) {
		storage->add_event(p_emitter_name, p_event);
	}
};

/// Utility that allow to fetch the events from the world. You can even use it
/// in a C++ system like this:
/// ```
/// void my_emitter_system(Events<MyEvent, EMITTER(EmitterName1)> &p_events){
///		for(const MyEvent& event : p_events) {
///			event....;
///		}
/// }
/// ```
template <class E, typename EmitterName>
class EventsReceiver {
	const LocalVector<E> *emitter_storage = nullptr;

public:
	struct Iterator {
	private:
		const LocalVector<E> *emitter_storage = nullptr;

	public:
		using iterator_category = std::forward_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = const E *;
		uint32_t index = 0;

		Iterator(const LocalVector<E> *p_emitter_storage, uint32_t p_index) :
				emitter_storage(p_emitter_storage),
				index(p_index) {}

		bool is_valid() const {
			return index < emitter_storage->size();
		}

		value_type operator*() const {
			return emitter_storage->ptr() + index;
		}

		Iterator &operator++() {
			index += 1;
			return *this;
		}

		Iterator operator++(int) {
			Iterator tmp = *this;
			++(*this);
			return tmp;
		}

		friend bool operator==(const Iterator &a, const Iterator &b) { return a.index == b.index; }
		friend bool operator!=(const Iterator &a, const Iterator &b) { return a.index != b.index; }
	};

public:
	void initiate_process(World *p_world) {
		EventStorage<E> *storage = p_world->get_events_storage<E>();
		if (storage != nullptr) {
			emitter_storage = storage->get_events(get_emitter_name());
		}
	}

	void release_world() {
		emitter_storage = nullptr;
	}

	static String get_emitter_name() {
		return String(EmitterName::data());
	}

	/// Returns the forward iterator to fetch the events.
	Iterator begin() {
		ERR_FAIL_COND_V_MSG(emitter_storage == nullptr, Iterator(emitter_storage, 0), "The emitter `" + E::get_class_static() + "::" + get_emitter_name() + "` storage doesn't exist.");
		return Iterator(emitter_storage, 0);
	}

	/// Used to know the last element of the `Iterator`.
	Iterator end() {
		ERR_FAIL_COND_V_MSG(emitter_storage == nullptr, Iterator(emitter_storage, 0), "The emitter `" + E::get_class_static() + "::" + get_emitter_name() + "` storage doesn't exist.");
		return Iterator(emitter_storage, emitter_storage->size());
	}
};
