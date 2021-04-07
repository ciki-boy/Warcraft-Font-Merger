#pragma once

#include "sfnt.h"

namespace otfcc {
struct font;
};

#include "glyph-order.hpp"

#if defined(OTFCC_ENABLE_VARIATION) && OTFCC_ENABLE_VARIATION
#include "table/fvar.h"
#endif

#include "table/cff_.hpp"
#include "table/os_2.hpp"
#include "table/glyf.hpp"
#include "table/hdmx.hpp"
#include "table/head.hpp"
#include "table/hhea.hpp"
#include "table/hmtx.hpp"
#include "table/maxp.hpp"
#include "table/meta.h"
#include "table/name.h"
#include "table/post.h"
#include "table/vhea.h"
#include "table/vmtx.hpp"

#include "table/VDMX.h"
#include "table/cmap.h"
#include "table/cvt.h"
#include "table/fpgm-prep.h"
#include "table/gasp.h"

#include "table/LTSH.h"
#include "table/VORG.h"

#include "table/BASE.h"
#include "table/GDEF.h"
#include "table/otl.h"

#include "table/COLR.h"
#include "table/CPAL.h"
#include "table/SVG.h"

#include "table/TSI5.h"
#include "table/_TSI.h"

struct font {

	enum class outline { ttf, cff };

	outline subtype;

#if defined(OTFCC_ENABLE_VARIATION) && OTFCC_ENABLE_VARIATION
	table_fvar *fvar;
#endif

	table_head *head;
	table_hhea *hhea;
	table_maxp *maxp;
	table_OS_2 *OS_2;
	table_hmtx *hmtx;
	table_post *post;
	table_hdmx *hdmx;

	table_vhea *vhea;
	table_vmtx *vmtx;
	table_VORG *VORG;

	table_CFF *CFF_;
	table_glyf *glyf;
	table_cmap *cmap;
	table_name *name;
	table_meta *meta;

	table_fpgm_prep *fpgm;
	table_fpgm_prep *prep;
	table_cvt *cvt_;
	table_gasp *gasp;
	table_VDMX *VDMX;

	table_LTSH *LTSH;

	table_OTL *GSUB;
	table_OTL *GPOS;
	table_GDEF *GDEF;
	table_BASE *BASE;

	table_CPAL *CPAL;
	table_COLR *COLR;
	table_SVG *SVG_;

	table_TSI *TSI_01;
	table_TSI *TSI_23;
	table_TSI5 *TSI5;

	otfcc_GlyphOrder *glyph_order;
};

extern caryll_ElementInterfaceOf(otfcc_Font) {
	caryll_RT(otfcc_Font);
	void (*consolidate)(otfcc_Font * font, const otfcc_Options *options);
	void *(*createTable)(otfcc_Font * font, const uint32_t tag);
	void (*deleteTable)(otfcc_Font * font, const uint32_t tag);
}
otfcc_iFont;

// Font builder interfaces
typedef struct otfcc_IFontBuilder {
	otfcc_Font *(*read)(void *source, uint32_t index, const otfcc_Options *options);
	void (*free)(struct otfcc_IFontBuilder *self);
} otfcc_IFontBuilder;
otfcc_IFontBuilder *otfcc_newOTFReader();
otfcc_IFontBuilder *otfcc_newJsonReader();

// Font serializer interface
typedef struct otfcc_IFontSerializer {
	void *(*serialize)(otfcc_Font *font, const otfcc_Options *options);
	void (*free)(struct otfcc_IFontSerializer *self);
} otfcc_IFontSerializer;
otfcc_IFontSerializer *otfcc_newJsonWriter();
otfcc_IFontSerializer *otfcc_newOTFWriter();
