#pragma once

#include <vector>

#include "../handle.hpp"

namespace otfcc::table::tsi_ {

struct entry {

	enum class type {
		glyph,
		fpgm,
		prep,
		cvt,
		reserved_fffc,
	};

	type type;
	glyph_handle glyph;
	std::string content;
};

using table = std::vector<entry>;

} // namespace otfcc::table::tsi
