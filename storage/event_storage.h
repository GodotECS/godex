#pragma once

#include "core/templates/local_vector.h"
#include "core/templates/oa_hash_map.h"
#include "core/variant/dictionary.h"

enum EventClearMode {
	EVENT_CLEAR_MODE_ON_FETCH,
	EVENT_CLEAR_MODE_END_OF_FRAME,
};

class EventStorageBase {
public:
	virtual void add_event_emitter(const StringName &p_emitter) {
		CRASH_NOW_MSG("Override this function.");
	}

	virtual void add_event_dynamic(const StringName &p_emitter, const Dictionary &p_data) {
		CRASH_NOW_MSG("Override this function.");
	}

	// TODO add dynamic read to allow GDScript systems to fetch events.

	virtual void flush_emitter_events(const StringName &p_emitter) {
		CRASH_NOW_MSG("Override this function.");
	}
};

template <class E>
class EventStorage : public EventStorageBase {
	OAHashMap<StringName, LocalVector<E>> events_map;

public:
	virtual void add_event_emitter(const StringName &p_emitter) override {
		ERR_FAIL_COND_MSG(events_map.has(p_emitter), "The emitter `" + p_emitter + "` for the event `" + E::event_name() + "` exists.");
		events_map.insert(p_emitter, LocalVector<E>());
	}

	virtual void add_event_dynamic(const StringName &p_emitter, const Dictionary &p_data) override {
		E insert_data;

		// Set the custom data if any.
		for (const Variant *key = p_data.next(); key; key = p_data.next(key)) {
			E::set_by_name((void *)&insert_data, key->operator StringName(), *p_data.getptr(*key));
		}

		add_event(p_emitter, insert_data);
	}

	virtual void flush_emitter_events(const StringName &p_emitter) override {
		LocalVector<E> *emitter = events_map.lookup_ptr(p_emitter);
		ERR_FAIL_COND_MSG(emitter == nullptr, "The emitter `" + p_emitter + "` for the event `" + E::event_name() + "` doesn't exists.");
		emitter->clear();
	}

public:
	void add_event(const StringName &p_emitter, E p_event) {
		LocalVector<E> *emitter = events_map.lookup_ptr(p_emitter);
		ERR_FAIL_COND_MSG(emitter == nullptr, "The emitter `" + p_emitter + "` for the event `" + E::event_name() + "` doesn't exists.");
		emitter->push_back(p_event);
	}

	const LocalVector<E> *get_events(const StringName &p_emitter, E p_event) const {
		return events_map.lookup_ptr(p_emitter);
	}
};