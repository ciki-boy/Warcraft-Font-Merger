#pragma once

#include <cstdint>
#include <string>

#include "primitives.hpp"

namespace otfcc {

struct handle {

	enum state_t {
		HANDLE_STATE_EMPTY = 0,
		HANDLE_STATE_INDEX = 1,
		HANDLE_STATE_NAME = 2,
		HANDLE_STATE_CONSOLIDATED = 3,
	};

	state_t state;
	glyphid_t index;
	std::string name;

	handle() : state(), index(), name() {}
	handle(glyphid_t gid) : handle(gid, std::string(), HANDLE_STATE_INDEX) {}
	handle(std::string name)
	    : state(name.length() ? HANDLE_STATE_NAME : HANDLE_STATE_EMPTY), index(0), name(name) {}
	// from consolidated
	handle(glyphid_t gid, std::string name)
	    : state(HANDLE_STATE_CONSOLIDATED), index(gid), name(name) {}
	handle(glyphid_t gid, std::string name, state_t state) : state(state), index(gid), name(name) {}

	void reset() { *this = handle{}; }
};

struct glyph_handle : handle {
	glyph_handle(const handle &handle_) : handle(handle_) {}
};

struct fd_handle : handle {
	fd_handle(const handle &handle_) : handle(handle_) {}
};

struct lookup_handle : handle {
	lookup_handle(const handle &handle_) : handle(handle_) {}
};

struct axis_handle : handle {
	axis_handle(const handle &handle_) : handle(handle_) {}
};

} // namespace otfcc
