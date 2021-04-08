#pragma once

#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace otfcc {

struct buffer_view : std::string_view {

	using base = std::string_view;
	size_t _cursor;

	buffer_view(const char *data, size_t length) : base(data, length) {}
	buffer_view(const std::string_view &view) : base(view) {}
	buffer_view(const buffer_view &) = default;
	~buffer_view() = default;
	buffer_view &operator=(const buffer_view &) = default;

	operator bool() const { return empty(); }
	size_t pos() const { return _cursor; }
	void seek(size_t pos) { _cursor = pos; }

	uint8_t read8u() { return uint8_t((*this)[_cursor++]); }
	uint16_t read16u() {
		uint16_t b0 = read8u();
		uint16_t b1 = read8u();
		return (b0 << 8) | b1;
	}
	uint32_t read24u() {
		uint32_t b0 = read8u();
		uint32_t b1 = read8u();
		uint32_t b2 = read8u();
		return (b0 << 16) | (b1 << 8) | b2;
	}
	uint32_t read32u() {
		uint32_t w0 = read16u();
		uint32_t w1 = read16u();
		return (w0 << 16) | w1;
	}
	uint64_t read64u() {
		uint64_t dw0 = read32u();
		uint64_t dw1 = read32u();
		return (dw0 << 32) | dw1;
	}
	int8_t read8s() { return read8u(); }
	int16_t read16s() { return read16u(); }
	int32_t read32s() { return read32u(); }
	int64_t read64s() { return read64u(); }
};

struct buffer : std::string {

	using base = std::string;
	size_t _cursor;

	buffer() : _cursor(0) {}
	buffer(std::initializer_list<value_type> il) : base(il), _cursor(il.size()) {}
	buffer(const buffer &) = default;
	buffer(buffer &&) = default;
	~buffer() = default;
	buffer &operator=(const buffer &) = default;
	buffer &operator=(buffer &&) = default;

	operator bool() const { return empty(); }
	size_t pos() const { return _cursor; }
	void seek(size_t pos) { _cursor = pos; }

	void write8(uint8_t x) {
		if (_cursor == size()) [[likely]] {
			push_back(x);
			_cursor++;
		} else [[unlikely]]
			(*this)[_cursor++] = x;
	}
	void write16(uint16_t x) {
		write8((x >> 8) & 0xff);
		write8(x & 0xff);
	}
	void write24(uint32_t x) {
		write8((x >> 16) & 0xff);
		write8((x >> 8) & 0xff);
		write8(x & 0xff);
	}
	void write32(uint32_t x) {
		write16((x >> 16) & 0xffff);
		write16(x & 0xffff);
	}
	void write64(uint64_t x) {
		write32((x >> 32) & 0xffffffff);
		write32(x & 0xffffffff);
	}

	template <typename... Ts> void nwrite8(Ts... args) { (write8(args), ...); }

	void write(const std::string &str) {
		for (char ch : str)
			write8(ch);
	}
	void write(const char *str) {
		while (char ch = *str++) {
			write8(ch);
		}
	}
	void write(const char *data, size_t len) {
		for (size_t i = 0; i < len; i++)
			write8(data[i]);
	}
	void write(const buffer &buf) {
		for (uint8_t byte : buf)
			write8(byte);
	}

	void append8(uint8_t x) {
		push_back(x);
		_cursor++;
	}
	void append16(uint16_t x) {
		append8((x >> 8) & 0xff);
		append8(x & 0xff);
	}
	void append24(uint32_t x) {
		append8((x >> 16) & 0xff);
		append8((x >> 8) & 0xff);
		append8(x & 0xff);
	}
	void append32(uint32_t x) {
		append16((x >> 16) & 0xffff);
		append16(x & 0xffff);
	}
	void append64(uint64_t x) {
		append32((x >> 32) & 0xffffffff);
		append32(x & 0xffffffff);
	}

	template <typename... Ts> void nappend8(Ts... args) { (append8(args), ...); }

	/* `append` functions are implicitly inherited from std::string */

	void align4() {
		size_t cp = pos();
		seek(size());
		switch (size() % 4) {
		case 1:
			append8(0);
			[[fallthrough]];
		case 2:
			append8(0);
			[[fallthrough]];
		case 3:
			append8(0);
			[[fallthrough]];
		default:
			break;
		}
		seek(cp);
	}

	// bufpingpong16 writes a buffer and an offset towards it.
	// [ ^                            ] + ###### that
	//   ^cp             ^offset
	//                           |
	//                           V
	// [ @^              ######       ] , and the value of [@] equals to the former offset.
	//    ^cp                  ^offset
	// Common in writing OpenType features.
	void ping16(size_t &offset, size_t &cp) {
		write16(offset);
		cp = pos();
		seek(offset);
	}
	void ping16d(size_t &offset, size_t &shift, size_t &cp) {
		write16(offset - shift);
		cp = pos();
		seek(offset);
	}
	void pong(size_t &offset, size_t &cp) {
		offset = pos();
		seek(cp);
	}
	void pingpong16(buffer &buf, size_t &offset, size_t &cp) {
		write16(offset);
		cp = pos();
		seek(offset);
		write(buf);
		buf.clear();
		offset = pos();
		seek(cp);
	}

	void print() const {
		for (size_t i = 0; i < size(); i++) {
			if (i % 16)
				fprintf(stderr, " ");
			fprintf(stderr, "%02x", data()[i]);
			if (i % 16 == 15)
				fprintf(stderr, "\n");
		}
		fprintf(stderr, "\n");
	}
};

} // namespace otfcc
