#pragma once

#include "dep/json.h"

#include "../buffer.hpp"
#include "../handle.hpp"
#include "../glyph-order.hpp"
#include "../primitives.hpp"
#include "../sfnt.h"
#include "../options.hpp"
#include <bits/stdint-intn.h>

#if defined (OTFCC_ENABLE_VARIATION) && OTFCC_ENABLE_VARIATION
#include "otfcc/vf/vq.h"
#endif

namespace otfcc::table {

namespace exception {

struct too_short {
	const char *name;
	const char *info;
	size_t expected;
	size_t actual;
};

struct unknown_format {
	const char *name;
	int32_t format;
};

}

}
