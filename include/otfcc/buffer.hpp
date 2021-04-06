#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace otfcc {

class buffer {
  public:
	using value_type = char;
	using pointer = value_type *;
	using const_pointer = const value_type *;
	using iterator = std::vector<value_type>::iterator;
	using const_iterator = std::vector<value_type>::const_iterator;

  private:
	std::vector<value_type> _data;
	size_t _cursor;

  public:
	buffer() : _cursor(0) {}
	buffer(std::initializer_list<value_type> il) : _data(il), _cursor(il.size()) {}
	buffer(const buffer &) = default;
	buffer(buffer &&) = default;
	~buffer() = default;
	buffer &operator=(const buffer &) = default;
	buffer &operator=(buffer &&) = default;

	bool empty() const { return _data.empty(); }
	operator bool() const { return _data.empty(); }
	size_t size() const { return _data.size(); }
	size_t length() const { return _data.size(); }
	size_t pos() const { return _cursor; }
	void seek(size_t pos) { _cursor = pos; }
	void clear() { _data.clear(); }

	pointer data() { return _data.data(); }
	const_pointer data() const { return _data.data(); }
	iterator begin() { return _data.begin(); }
	const_iterator begin() const { return _data.begin(); }
	iterator end() { return _data.end(); }
	const_iterator end() const { return _data.end(); }
	const_iterator cbegin() const { return _data.cbegin(); }
	const_iterator cend() const { return _data.cend(); }

	void write8(uint8_t x) {
		if (_cursor == _data.size()) [[likely]] {
			_data.push_back(x);
			_cursor++;
		} else [[unlikely]]
			_data[_cursor++] = x;
	}
	void write16l(uint16_t x) {
		write8(x & 0xff);
		write8((x >> 8) & 0xff);
	}
	void write16b(uint16_t x) {
		write8((x >> 8) & 0xff);
		write8(x & 0xff);
	}
	void write24l(uint32_t x) {
		write8(x & 0xff);
		write8((x >> 8) & 0xff);
		write8((x >> 16) & 0xff);
	}
	void write24b(uint32_t x) {
		write8((x >> 16) & 0xff);
		write8((x >> 8) & 0xff);
		write8(x & 0xff);
	}
	void write32l(uint32_t x) {
		write8(x & 0xff);
		write8((x >> 8) & 0xff);
		write8((x >> 16) & 0xff);
		write8((x >> 24) & 0xff);
	}
	void write32b(uint32_t x) {
		write8((x >> 24) & 0xff);
		write8((x >> 16) & 0xff);
		write8((x >> 8) & 0xff);
		write8(x & 0xff);
	}
	void write64l(uint64_t x) {
		write8(x & 0xff);
		write8((x >> 8) & 0xff);
		write8((x >> 16) & 0xff);
		write8((x >> 24) & 0xff);
		write8((x >> 32) & 0xff);
		write8((x >> 40) & 0xff);
		write8((x >> 48) & 0xff);
		write8((x >> 56) & 0xff);
	}
	void write64b(uint64_t x) {
		write8((x >> 56) & 0xff);
		write8((x >> 48) & 0xff);
		write8((x >> 40) & 0xff);
		write8((x >> 32) & 0xff);
		write8((x >> 24) & 0xff);
		write8((x >> 16) & 0xff);
		write8((x >> 8) & 0xff);
		write8(x & 0xff);
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
	void write(const char *buf, size_t len) {
		for (size_t i = 0; i < len; i++)
			write8(buf[i]);
	}
	void write(const buffer &buf) {
		for (uint8_t byte : buf)
			write8(byte);
	}

	void align4() {
		size_t cp = pos();
		seek(size());
		switch (size() % 4) {
		case 1:
			write8(0);
			[[fallthrough]];
		case 2:
			write8(0);
			[[fallthrough]];
		case 3:
			write8(0);
			[[fallthrough]];
		default:
			break;
		}
		seek(cp);
	}

	// bufpingpong16b writes a buffer and an offset towards it.
	// [ ^                            ] + ###### that
	//   ^cp             ^offset
	//                           |
	//                           V
	// [ @^              ######       ] , and the value of [@] equals to the former offset.
	//    ^cp                  ^offset
	// Common in writing OpenType features.
	void ping16b(size_t &offset, size_t &cp) {
		write16b(offset);
		cp = pos();
		seek(offset);
	}
	void ping16bd(size_t &offset, size_t &shift, size_t &cp) {
		write16b(offset - shift);
		cp = pos();
		seek(offset);
	}
	void pong(size_t &offset, size_t &cp) {
		offset = pos();
		seek(cp);
	}
	void pingpong16b(buffer &buf, size_t &offset, size_t &cp) {
		write16b(offset);
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

} // namespace caryll
