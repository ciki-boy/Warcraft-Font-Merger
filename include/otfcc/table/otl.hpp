#pragma once

#include <algorithm>
#include <bits/stdint-uintn.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <set>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "otfcc/primitives.h"
#include "otfcc/primitives.hpp"
#include "table-common.h"

namespace otfcc::table {

struct coverage : std::vector<glyph_handle> {
	using base = std::vector<glyph_handle>;

	coverage(buffer_view buf) {
		static char info[4096];
		if (buf.length() < 4)
			throw exception::too_short{
			    .name = "otl::coverage",
			    .info = "reading format and glyphCount",
			    .expected = 4,
			    .actual = buf.length(),
			};
		uint16_t format = buf.read16u();
		switch (format) {
		case 1: {
			uint16_t glyphCount = buf.read16u();
			if (buf.length() < 4 + glyphCount * 2) {
				snprintf(info, sizeof info, "glyphCount = %d", glyphCount);
				throw exception::too_short{
				    .name = "otl::coverage",
				    .info = info,
				    .expected = size_t(4 + glyphCount * 2),
				    .actual = buf.length(),
				};
			}
			std::set<glyphid_t> occupied;
			for (uint16_t j = 0; j < glyphCount; j++) {
				glyphid_t gid = buf.read16u();
				if (occupied.find(gid) == occupied.end()) {
					occupied.insert(gid);
					push_back(handle{gid});
				}
			}
			break;
		}
		case 2: {
			uint16_t rangeCount = buf.read16u();
			if (buf.length() < 4 + rangeCount * 6) {
				snprintf(info, sizeof info, "rangeCount = %d", rangeCount);
				throw exception::too_short{
				    .name = "otl::coverage",
				    .info = info,
				    .expected = size_t(4 + rangeCount * 6),
				    .actual = buf.length(),
				};
			}
			std::set<glyphid_t> occupied;
			for (uint16_t j = 0; j < rangeCount; j++) {
				glyphid_t start = buf.read16u();
				glyphid_t end = buf.read16u();
				glyphid_t startCoverageIndex = buf.read16u();
				for (int32_t k = start; k <= end; k++) {
					glyphid_t gid = k;
					if (occupied.find(gid) == occupied.end()) {
						occupied.insert(gid);
						push_back(handle{gid});
					}
				}
			}
			break;
		}
		default:
			throw exception::unknown_format{
			    .name = "otl::coverage",
			    .format = format,
			};
		}
	}

	coverage(const nlohmann::json &json) {
		if (!json.is_array())
			return;
		for (const nlohmann::json &name : json) {
			if (name.is_string())
				push_back(handle{std::string(name)});
		}
	}

	void consolidate(const glyph_order &gord) {
		std::vector<glyph_handle> consolidated;
		for (glyph_handle &h : *this) {
			if (gord.consolidate_handle(h))
				consolidated.push_back(h);
		}
		std::sort(consolidated.begin(), consolidated.end(),
		          [](const handle &a, const handle &b) { return a.index < b.index; });
		auto it = std::unique(consolidated.begin(), consolidated.end(),
		                      [](const handle &a, const handle &b) { return a.index == b.index; });
		consolidated.resize(it - consolidated.begin());
		swap(consolidated);
	}

	// assume consolidated
	nlohmann::json to_json() const {
		nlohmann::json result = nlohmann::json::array();
		std::transform(begin(), end(), std::back_inserter(result),
		               [](const glyph_handle &h) { return h.name; });
		return result;
	}

	buffer build_format1() const {
		buffer result;
		result.append16(1);
		result.append16(size());
		for (const glyph_handle &h : *this)
			result.append16(h.index);
		return result;
	}

	buffer build_format2() const {
		buffer result;
		result.append16(2);
		buffer ranges;
		glyphid_t startGid = begin()->index;
		glyphid_t endGid = startGid;
		glyphid_t lastGid = startGid;
		size_t nRanges = 0;
		for (size_t j = 1; j < size(); j++) {
			glyphid_t current = (*this)[j].index;
			if (current <= lastGid)
				continue;
			if (current == endGid + 1)
				endGid = current;
			else {
				ranges.append16(startGid);
				ranges.append16(endGid);
				ranges.append16(j + startGid - endGid - 1);
				nRanges += 1;
				startGid = endGid = current;
			}
			lastGid = current;
		}
		ranges.append16(startGid);
		ranges.append16(endGid);
		ranges.append16(size() + startGid - endGid - 1);
		nRanges += 1;
		result.append16(nRanges);
		result.append(ranges);
		return result;
	}

	// assume consolidated
	buffer build(uint16_t format = 0) const {
		switch (format) {
		case 1:
			return build_format1();
		case 2:
			return build_format2();
		default:
			auto format1 = build_format1();
			auto format2 = build_format2();
			if (format1.length() <= format2.length())
				return format1;
			else
				return format2;
		}
	}
};

struct class_definition : std::vector<std::pair<glyph_handle, glyphclass_t>> {
	glyphclass_t maxclass;

	using base = std::vector<std::pair<glyph_handle, glyphclass_t>>;

	class_definition(buffer_view buf) {
		static char info[4096];
		if (buf.length() < 4)
			throw exception::too_short{
			    .name = "otl::class_definition",
			    .info = "reading format and count",
			    .expected = 4,
			    .actual = buf.length(),
			};
		uint16_t format = buf.read16u();
		switch (format) {
		case 1: {
			if (buf.length() < 6)
				throw exception::too_short{
				    .name = "otl::class_definition",
				    .info = "reading format 1 startGlyphID and glyphCount",
				    .expected = 6,
				    .actual = buf.length(),
				};
			uint16_t startGid = buf.read16u();
			uint16_t count = buf.read16u();
			if (buf.length() < 6 + count * 2) {
				snprintf(info, sizeof info, "glyphCount = %d", count);
				throw exception::too_short{
				    .name = "otl::class_definition",
				    .info = info,
				    .expected = size_t(6 + count * 2),
				    .actual = buf.length(),
				};
			}
			for (uint16_t j = 0; j < count; j++) {
				glyphclass_t klass = buf.read16u();
				emplace_back(handle{glyphid_t(startGid + j)}, klass);
			}
			break;
		}
		case 2: {
			uint16_t rangeCount = buf.read16u();
			if (buf.length() < 4 + rangeCount * 6) {
				snprintf(info, sizeof info, "rangeCount = %d", rangeCount);
				throw exception::too_short{
				    .name = "otl::class_definition",
				    .info = info,
				    .expected = size_t(4 + rangeCount * 6),
				    .actual = buf.length(),
				};
			}
			// the ranges may overlap.
			std::set<glyphid_t> occupied;
			for (uint16_t j = 0; j < rangeCount; j++) {
				glyphid_t start = buf.read16u();
				glyphid_t end = buf.read16u();
				glyphid_t klass = buf.read16u();
				for (int32_t k = start; k <= end; k++) {
					glyphid_t gid = k;
					if (occupied.find(gid) == occupied.end()) {
						occupied.insert(gid);
						emplace_back(handle{gid}, klass);
					}
				}
			}
			break;
		}
		default:
			throw exception::unknown_format{
			    .name = "otl::coverage",
			    .format = format,
			};
		}
	}

	class_definition(const nlohmann::json &json) {
		if (!json.is_object())
			return;
		for (const auto &[name, klass] : json.items()) {
			if (klass.is_number())
				emplace_back(handle{name}, glyphclass_t(klass));
		}
	}

	void extend(const coverage &cov) {
		std::set<glyphid_t> occupied;
		std::transform(begin(), end(), std::inserter(occupied, occupied.end()),
		               [](const auto &item) { return item.first.index; });
		for (const handle &h : cov)
			if (occupied.find(h.index) == occupied.end()) {
				occupied.insert(h.index);
				emplace_back(h, 0);
			}
	}

	void consolidate(const glyph_order &gord) {
		std::vector<std::pair<glyph_handle, glyphclass_t>> consolidated;
		for (auto &[handle, klass] : *this) {
			if (gord.consolidate_handle(handle))
				consolidated.emplace_back(handle, klass);
		}
		std::sort(consolidated.begin(), consolidated.end(),
		          [](const auto &a, const auto &b) { return a.first.index < b.first.index; });
		auto it =
		    std::unique(consolidated.begin(), consolidated.end(), [](const auto &a, const auto &b) {
			    return a.first.index == b.first.index;
		    });
		consolidated.resize(it - consolidated.begin());
		swap(consolidated);
	}

	// assume consolidated
	nlohmann::json to_json() const {
		nlohmann::json result;
		std::transform(begin(), end(), std::inserter(result, result.end()),
		               [](const auto &h) { return h.first.name; });
		return result;
	}

	// assume consolidated
	buffer build() const {
		buffer result;
		result.append16(2);
		if (!size()) {
			result.append16(0);
			return result;
		}

		struct record {
			glyphid_t gid;
			glyphclass_t klass;
		};
		std::vector<record> nonTrivial;
		for (const auto &[handle, klass] : *this)
			if (klass)
				nonTrivial.emplace_back(handle.index, klass);
		if (!nonTrivial.size()) {
			// the class def has only class 0
			result.append16(0);
			return result;
		}
		std::sort(nonTrivial.begin(), nonTrivial.end(),
		          [](record a, record b) { return a.gid < b.gid; });

		glyphid_t startGid = nonTrivial[0].gid;
		glyphid_t endGid = startGid;
		glyphclass_t lastClass = nonTrivial[0].klass;
		size_t nRanges = 0;
		glyphid_t lastGid = startGid;
		buffer ranges;
		for (size_t j = 1; j < nonTrivial.size(); j++) {
			glyphid_t current = nonTrivial[j].gid;
			if (current <= lastGid)
				continue;
			if (current == endGid + 1 && nonTrivial[j].klass == lastClass)
				endGid = current;
			else {
				ranges.append16(startGid);
				ranges.append16(endGid);
				ranges.append16(lastClass);
				nRanges += 1;
				startGid = endGid = current;
				lastClass = nonTrivial[j].klass;
			}
			lastGid = current;
		}
		ranges.append16(startGid);
		ranges.append16(endGid);
		ranges.append16(lastClass);
		nRanges += 1;
		result.append16(nRanges);
		result.append(ranges);
	}
};

enum class lookup_type {
	unknown = 0,

	gsub_unknown = 0x10,
	gsub_single = 0x11,
	gsub_multiple = 0x12,
	gsub_alternate = 0x13,
	gsub_ligature = 0x14,
	gsub_context = 0x15,
	gsub_chaining = 0x16,
	gsub_extend = 0x17,
	gsub_reverse = 0x18,

	gpos_unknown = 0x20,
	gpos_single = 0x21,
	gpos_pair = 0x22,
	gpos_cursive = 0x23,
	gpos_markToBase = 0x24,
	gpos_markToLigature = 0x25,
	gpos_markToMark = 0x26,
	gpos_context = 0x27,
	gpos_chaining = 0x28,
	gpos_extend = 0x29,
};

typedef union _otl_subtable otl_Subtable;
typedef struct _otl_lookup otl_Lookup;

struct postion_value {
	pos_t dx;
	pos_t dy;
	pos_t dWidth;
	pos_t dHeight;
};

} // namespace otfcc::table

// GSUB subtable formats
typedef struct {
	OWNING otfcc_GlyphHandle from;
	OWNING otfcc_GlyphHandle to;
} otl_GsubSingleEntry;
typedef caryll_Vector(otl_GsubSingleEntry) subtable_gsub_single;
extern caryll_VectorInterface(subtable_gsub_single, otl_GsubSingleEntry) iSubtable_gsub_single;

typedef struct {
	OWNING otfcc_GlyphHandle from;
	OWNING otl_Coverage *to;
} otl_GsubMultiEntry;
typedef caryll_Vector(otl_GsubMultiEntry) subtable_gsub_multi;
extern caryll_VectorInterface(subtable_gsub_multi, otl_GsubMultiEntry) iSubtable_gsub_multi;

typedef struct {
	OWNING otl_Coverage *from;
	OWNING otfcc_GlyphHandle to;
} otl_GsubLigatureEntry;
typedef caryll_Vector(otl_GsubLigatureEntry) subtable_gsub_ligature;
extern caryll_VectorInterface(subtable_gsub_ligature,
                              otl_GsubLigatureEntry) iSubtable_gsub_ligature;

typedef enum {
	otl_chaining_canonical =
	    0, // The canonical form of chaining contextual substitution, one rule per subtable.
	otl_chaining_poly = 1, // The multi-rule form, right after reading OTF. N rule per subtable.
	otl_chaining_classified =
	    2 // The classified intermediate form, for building TTF with compression.
	      // N rules, has classdefs, and coverage GID interpreted as class number.
} otl_chaining_type;

typedef struct {
	tableid_t index;
	otfcc_LookupHandle lookup;
} otl_ChainLookupApplication;
typedef struct {
	tableid_t matchCount;
	tableid_t inputBegins;
	tableid_t inputEnds;
	OWNING otl_Coverage **match;
	tableid_t applyCount;
	OWNING otl_ChainLookupApplication *apply;
} otl_ChainingRule;
typedef struct {
	otl_chaining_type type;
	union {
		otl_ChainingRule rule; // for type = otl_chaining_canonical
		struct {               // for type = otl_chaining_poly or otl_chaining_classified
			tableid_t rulesCount;
			OWNING otl_ChainingRule **rules;
			OWNING otl_ClassDef *bc;
			OWNING otl_ClassDef *ic;
			OWNING otl_ClassDef *fc;
		};
	};
} subtable_chaining;
extern caryll_RefElementInterface(subtable_chaining) iSubtable_chaining;

typedef struct {
	tableid_t matchCount;
	tableid_t inputIndex;
	OWNING otl_Coverage **match;
	OWNING otl_Coverage *to;
} subtable_gsub_reverse;
extern caryll_RefElementInterface(subtable_gsub_reverse) iSubtable_gsub_reverse;

// GPOS subtable formats
typedef struct {
	OWNING otfcc_GlyphHandle target;
	OWNING otl_PositionValue value;
} otl_GposSingleEntry;
typedef caryll_Vector(otl_GposSingleEntry) subtable_gpos_single;
extern caryll_VectorInterface(subtable_gpos_single, otl_GposSingleEntry) iSubtable_gpos_single;

typedef struct {
	bool present;
	pos_t x;
	pos_t y;
} otl_Anchor;

typedef struct {
	OWNING otl_ClassDef *first;
	OWNING otl_ClassDef *second;
	OWNING otl_PositionValue **firstValues;
	OWNING otl_PositionValue **secondValues;
} subtable_gpos_pair;
extern caryll_RefElementInterface(subtable_gpos_pair) iSubtable_gpos_pair;

typedef struct {
	OWNING otfcc_GlyphHandle target;
	OWNING otl_Anchor enter;
	OWNING otl_Anchor exit;
} otl_GposCursiveEntry;
typedef caryll_Vector(otl_GposCursiveEntry) subtable_gpos_cursive;
extern caryll_VectorInterface(subtable_gpos_cursive, otl_GposCursiveEntry) iSubtable_gpos_cursive;

typedef struct {
	OWNING otfcc_GlyphHandle glyph;
	glyphclass_t markClass;
	otl_Anchor anchor;
} otl_MarkRecord;
typedef caryll_Vector(otl_MarkRecord) otl_MarkArray;
extern caryll_VectorInterface(otl_MarkArray, otl_MarkRecord) otl_iMarkArray;

typedef struct {
	OWNING otfcc_GlyphHandle glyph;
	OWNING otl_Anchor *anchors;
} otl_BaseRecord;
typedef caryll_Vector(otl_BaseRecord) otl_BaseArray;
extern caryll_VectorInterface(otl_BaseArray, otl_BaseRecord) otl_iBaseArray;

typedef struct {
	glyphclass_t classCount;
	OWNING otl_MarkArray markArray;
	otl_BaseArray baseArray;
} subtable_gpos_markToSingle;
extern caryll_RefElementInterface(subtable_gpos_markToSingle) iSubtable_gpos_markToSingle;

typedef struct {
	OWNING otfcc_GlyphHandle glyph;
	glyphid_t componentCount;
	OWNING otl_Anchor **anchors;
} otl_LigatureBaseRecord;
typedef caryll_Vector(otl_LigatureBaseRecord) otl_LigatureArray;
extern caryll_VectorInterface(otl_LigatureArray, otl_LigatureBaseRecord) otl_iLigatureArray;

typedef struct {
	glyphclass_t classCount;
	OWNING otl_MarkArray markArray;
	otl_LigatureArray ligArray;
} subtable_gpos_markToLigature;
extern caryll_RefElementInterface(subtable_gpos_markToLigature) iSubtable_gpos_markToLigature;

typedef struct {
	otl_LookupType type;
	otl_Subtable *subtable;
} subtable_extend;

typedef union _otl_subtable {
	subtable_gsub_single gsub_single;
	subtable_gsub_multi gsub_multi;
	subtable_gsub_ligature gsub_ligature;
	subtable_chaining chaining;
	subtable_gsub_reverse gsub_reverse;
	subtable_gpos_single gpos_single;
	subtable_gpos_pair gpos_pair;
	subtable_gpos_cursive gpos_cursive;
	subtable_gpos_markToSingle gpos_markToSingle;
	subtable_gpos_markToLigature gpos_markToLigature;
	subtable_extend extend;
} otl_Subtable;

typedef otl_Subtable *otl_SubtablePtr;
typedef caryll_Vector(otl_SubtablePtr) otl_SubtableList;
extern caryll_VectorInterfaceTypeName(otl_SubtableList) {
	caryll_VectorInterfaceTrait(otl_SubtableList, otl_SubtablePtr);
	void (*disposeDependent)(MODIFY otl_SubtableList *, const otl_Lookup *);
}
otl_iSubtableList;

struct _otl_lookup {
	sds name;
	otl_LookupType type;
	uint32_t _offset;
	uint16_t flags;
	OWNING otl_SubtableList subtables;
};

// owning lookup list
typedef OWNING otl_Lookup *otl_LookupPtr;
extern caryll_ElementInterface(otl_LookupPtr) otl_iLookupPtr;
typedef caryll_Vector(otl_LookupPtr) otl_LookupList;
extern caryll_VectorInterface(otl_LookupList, otl_LookupPtr) otl_iLookupList;

// observe lookup list
typedef OBSERVE otl_Lookup *otl_LookupRef;
extern caryll_ElementInterface(otl_LookupRef) otl_iLookupRef;
typedef caryll_Vector(otl_LookupRef) otl_LookupRefList;
extern caryll_VectorInterface(otl_LookupRefList, otl_LookupRef) otl_iLookupRefList;

typedef struct {
	sds name;
	OWNING otl_LookupRefList lookups;
} otl_Feature;
// owning feature list
typedef OWNING otl_Feature *otl_FeaturePtr;
extern caryll_ElementInterface(otl_FeaturePtr) otl_iFeaturePtr;
typedef caryll_Vector(otl_FeaturePtr) otl_FeatureList;
extern caryll_VectorInterface(otl_FeatureList, otl_FeaturePtr) otl_iFeatureList;
// observe feature list
typedef OBSERVE otl_Feature *otl_FeatureRef;
extern caryll_ElementInterface(otl_FeatureRef) otl_iFeatureRef;
typedef caryll_Vector(otl_FeatureRef) otl_FeatureRefList;
extern caryll_VectorInterface(otl_FeatureRefList, otl_FeatureRef) otl_iFeatureRefList;

typedef struct {
	sds name;
	OWNING otl_FeatureRef requiredFeature;
	OWNING otl_FeatureRefList features;
} otl_LanguageSystem;
typedef otl_LanguageSystem *otl_LanguageSystemPtr;
extern caryll_ElementInterface(otl_LanguageSystemPtr) otl_iLanguageSystem;
typedef caryll_Vector(otl_LanguageSystemPtr) otl_LangSystemList;
extern caryll_VectorInterface(otl_LangSystemList, otl_LanguageSystemPtr) otl_iLangSystemList;

typedef struct {
	otl_LookupList lookups;
	otl_FeatureList features;
	otl_LangSystemList languages;
} table_OTL;
extern caryll_RefElementInterface(table_OTL) table_iOTL;
