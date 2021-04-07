#pragma once

#include "dep/json.h"

#include "../buffer.hpp"
#include "../handle.hpp"
#include "../glyph-order.hpp"
#include "../primitives.hpp"
#include "../sfnt.h"
#include "../options.hpp"

#if defined (OTFCC_ENABLE_VARIATION) && OTFCC_ENABLE_VARIATION
#include "otfcc/vf/vq.h"
#endif
