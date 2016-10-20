#pragma once
#include <string>
#include <iostream>
#include <locale>
#include <codecvt>
#include <algorithm>
#include <type_traits>
class u8istreambuf final : public std::streambuf {
private:
	using Traits = traits_type;
	using wtraits = std::char_traits<wchar_t>;
	std::wstreambuf *s_buf_ptr;
	std::string buf;
	using wcvt_t = std::wstring_convert<typename std::conditional<sizeof(wchar_t) == 2, std::codecvt_utf8_utf16<wchar_t>, std::codecvt_utf8<wchar_t>>::type>;
	wchar_t read_surrogate_pairs_buf;
public:
	u8istreambuf() = delete;
	u8istreambuf(const u8istreambuf&) = delete;
	u8istreambuf(u8istreambuf&&) = delete;
	u8istreambuf& operator=(const u8istreambuf&) = delete;
	u8istreambuf& operator=(u8istreambuf&&) = delete;
	u8istreambuf(std::wstreambuf* s_ptr)
		: s_buf_ptr(s_ptr), buf(), read_surrogate_pairs_buf()
	{
		if (nullptr == this->s_buf_ptr) throw std::invalid_argument("wstreambuf is not bound.");
	}
private:
	static constexpr bool is_utf16_surrogate_pairs_first_element(wchar_t c) { return (2 == sizeof(wchar_t) && 0xD800 <= c && c <= 0xDBFF); }
	static std::string w2u8(const std::wstring& s) {
		static thread_local wcvt_t wcvt;
		return wcvt.to_bytes(s);
	}
	std::wstring read_wstream_buf(std::streamsize count = 1) {
		if (count < 0) throw std::out_of_range("utf8_streambuf");

		std::wstring tmp;

		//allocate
		tmp.reserve(static_cast<std::size_t>(count + 2));
		if (this->read_surrogate_pairs_buf) tmp = this->read_surrogate_pairs_buf;
		tmp.resize(static_cast<std::size_t>((this->read_surrogate_pairs_buf) ? count + 1 : count));
		//read
		const auto read_num = this->s_buf_ptr->sgetn(&tmp[0], count);
		if (read_num <= 0) return L"";
		tmp.resize(static_cast<std::size_t>(read_num));
		//get surrogate pairs 2nd element. we don't have to think about combining character.
		if (is_utf16_surrogate_pairs_first_element(tmp.back())) {
			if (wtraits::eq_int_type(wtraits::eof(), this->sgetc())) {
				this->read_surrogate_pairs_buf = tmp.back();
				tmp.pop_back();
			}
			tmp += wtraits::to_char_type(this->sbumpc());
		}
		return tmp;
	}
protected:
	//virtual void setg(char* eb, char* g, char *eg);
	//this->eback() : 1st argument of this->setg()
	//this->gptr()  : 2nd argument of this->setg()
	//this->egptr() : 3rd argument of this->setg()

	// get a character from stream, but don't point past it
	//The public functions of std::streambuf call this function only if gptr() == nullptr or gptr() >= egptr().
	virtual int_type underflow() override {
		//check buffer
		if (this->buf.empty()) {
			//read
			const auto tmp = this->read_wstream_buf();
			if (tmp.empty())  return Traits::eof();
			//convert
			this->buf = w2u8(tmp);//move
			if (this->buf.empty()) return 0;
		}
		return Traits::to_int_type(buf.front());
	}
	virtual int_type uflow() override {
		const auto re = this->underflow();
		this->buf.erase(0, 1);
		return re;
	}
	// put a character back to stream
	virtual int_type pbackfail(int_type c = Traits::eof()) override {
		if (c != Traits::eof()) {
			this->buf.insert(this->buf.begin(), c);
		}
		return c;
	}
	virtual std::streamsize xsgetn(char_type* s, std::streamsize count) override {
		if (count <= 0) return count;

		if (this->buf.empty()) {
			//read
			const auto tmp = this->read_wstream_buf(count);
			if (tmp.empty()) return 0;
			//convert
			this->buf = w2u8(tmp);//move
			if (this->buf.empty()) return 0;
		}
		else if (this->buf.size() < static_cast<std::size_t>(count)) {
			//read
			const auto tmp = this->read_wstream_buf(count - static_cast<std::streamsize>(buf.size()));
			//convert
			if (tmp.empty()) this->buf += w2u8(tmp);//copy back
		}
		const bool need_buf_erase = (static_cast<std::size_t>(count) < this->buf.size());
		const auto output_size = (!need_buf_erase) ? this->buf.size() : static_cast<std::size_t>(count);
		memcpy(s, this->buf.c_str(), output_size);
		if (need_buf_erase) {
			this->buf.erase(0, output_size);
		}
		else {
			this->buf.clear();
		}
		return 0;
	}
	virtual int_type overflow(int_type _Meta = Traits::eof()) override {
		throw std::runtime_error("u8istreambuf : output is not supported.");
	}
	virtual pos_type seekoff(
		off_type /*off*/, std::ios_base::seekdir /*dir*/,
		std::ios_base::openmode /*which*/ = std::ios_base::in | std::ios_base::out
	) override {
		return pos_type(-1);	// always fail
	}
	virtual pos_type seekpos(
		pos_type pos,
		std::ios_base::openmode which = std::ios_base::in | std::ios_base::out
	) override {
		//return this->seekoff(off_type(pos), std::ios_base::beg, which);
		return pos_type(-1);	// always fail
	}
};
class u8ostreambuf final : public std::streambuf {
private:
	using Traits = traits_type;
	using wtraits = std::char_traits<wchar_t>;
	std::wstreambuf *s_buf_ptr;
	std::string buf;
	using wcvt_t = std::wstring_convert<typename std::conditional<sizeof(wchar_t) == 2, std::codecvt_utf8_utf16<wchar_t>, std::codecvt_utf8<wchar_t>>::type>;
public:
	u8ostreambuf() = delete;
	u8ostreambuf(const u8ostreambuf&) = delete;
	u8ostreambuf(u8ostreambuf&&) = delete;
	u8ostreambuf& operator=(const u8ostreambuf&) = delete;
	u8ostreambuf& operator=(u8ostreambuf&&) = delete;
	u8ostreambuf(std::wstreambuf* s_ptr)
		: s_buf_ptr(s_ptr), buf()
	{
		if (nullptr == this->s_buf_ptr) throw std::invalid_argument("wstreambuf is not bound.");
		this->buf.reserve(120);
		this->buf.resize(100);
		this->setp(&buf[0], &buf.back());
	}
private:
	static std::wstring u82w(const std::string& s) {
		static thread_local wcvt_t wcvt;
		return wcvt.from_bytes(s);
	}
	static std::size_t how_many_utf8_byte_from_last_is_broken(const std::string& s) {
		//                     byte per codepoint :    2     3     4     5     6
		//                         rest charactor :    1     2     3     4     5
		static constexpr std::uint8_t mask_list[] = { 0xE0, 0xF0, 0xF8, 0xFC, 0xFE };
		static constexpr std::uint8_t comp_list[] = { 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
		static_assert(sizeof(mask_list) == sizeof(comp_list), "error");

		if (s.empty()) return 0;
		const auto iterate_num = std::min(s.size(), sizeof(mask_list) / sizeof(std::uint8_t));
		std::size_t i = 0;
		for (auto rit = s.rbegin(); i < iterate_num; ++i, ++rit) {
			for (std::size_t j = i; j < sizeof(mask_list) / sizeof(std::uint8_t); ++j) {
				if (comp_list[j] == (static_cast<std::uint8_t>(*rit) & mask_list[j])) return i + 1;
			}
		}
		return 0;
	}
	bool write_wstream_buf(bool rest_delete_flg = false) {
		if (this->buf.empty()) {
			this->buf.resize(100);
			this->setp(&this->buf[0], &this->buf.back());
			return true;
		}
		const auto rest_size = how_many_utf8_byte_from_last_is_broken(this->buf);
		const auto buf_size = this->buf.size();
		if (buf_size == rest_size) return true;
		const auto rest_front_pos = buf_size - rest_size;
		auto tmp = std::move(this->buf);
		this->buf.resize(100);
		if (!rest_delete_flg) {
			std::copy(tmp.begin() + rest_front_pos, tmp.end(), this->buf.begin());
			this->setp(&this->buf[rest_size], &this->buf.back());
		}
		else {
			this->setp(&this->buf[0], &this->buf.back());
		}
		tmp.erase(rest_front_pos);
		const auto converted = u82w(tmp);
		const auto size = static_cast<std::streamsize>(converted.size());
		return size == this->s_buf_ptr->sputn(converted.c_str(), size);
	}
protected:
	//virtual void setp(char* p, char* ep);
	//this->pbase() : 1st argument of this->setg()
	//this->epptr()  : 2nd argument of this->setg()

	//The sputc() and sputn() call this function in case of an overflow (pptr() == nullptr or pptr() >= epptr()).
	virtual int_type overflow(int_type _Meta = Traits::eof()) override {
		return write_wstream_buf() ? 0 : Traits::eof();
	}
	virtual int sync() override {
		this->buf.resize(Traits::length(this->buf.c_str()));
		const auto re =  write_wstream_buf(true) ? 0 : -1;
		this->s_buf_ptr->pubsync();
		return re;
	}
	virtual pos_type seekoff(
		off_type /*off*/, std::ios_base::seekdir /*dir*/,
		std::ios_base::openmode /*which*/ = std::ios_base::in | std::ios_base::out
	) override {
		return pos_type(-1);	// always fail
	}
	virtual pos_type seekpos(
		pos_type pos,
		std::ios_base::openmode which = std::ios_base::in | std::ios_base::out
	) override {
		//return this->seekoff(off_type(pos), std::ios_base::beg, which);
		return pos_type(-1);	// always fail
	}
};
class utf8_streambuf : public std::streambuf {
private:
	using Traits = traits_type;
	using wtraits = std::char_traits<wchar_t>;
	std::wstreambuf *s_buf_ptr;
	std::string read_buf;//utf-8 buffer
	std::string write_buf;
	using wcvt_t = std::wstring_convert<typename std::conditional<sizeof(wchar_t) == 2, std::codecvt_utf8_utf16<wchar_t>, std::codecvt_utf8<wchar_t>>::type>;
	wcvt_t wcvt;
	wchar_t read_surrogate_pairs_buf;
	enum class mode { writeable, read };
	enum class mode mode;
private:
	void change_mode(enum class mode m) {
		constexpr std::size_t write_buffer_size = 100;
		if (m == this->mode) return;
		if (m == mode::read) {
			this->setp(nullptr, nullptr);//update write ptr
		}
		else {
			std::size_t front_pos = 0;
			if (this->write_buf.empty()) {
				this->write_buf.resize(write_buffer_size);
			}
			else {
				front_pos = Traits::length(this->write_buf.c_str());
				if (this->write_buf.size() < write_buffer_size) {
					this->write_buf.resize(write_buffer_size);
				}
			}
			this->setp(&this->write_buf[front_pos], &this->write_buf[this->write_buf.length()]);//update write ptr
		}
		this->mode = m;
	}
public:
	utf8_streambuf(std::wstreambuf *pStream)
		: s_buf_ptr(pStream), read_buf(), write_buf(100, ' '), wcvt(), read_surrogate_pairs_buf(), mode(mode::read)
	{
		change_mode(mode::writeable);//update write ptr
	}
private:
	static constexpr bool is_utf16_surrogate_pairs_first_element(wchar_t c) { return (2 == sizeof(wchar_t) && 0xD800 <= c && c <= 0xDBFF); }
	std::wstring read_wstream_buf(std::streamsize count = 1) {
		if (count < 0) throw std::out_of_range("utf8_streambuf");
		std::wstring tmp;
		//allocate
		tmp.reserve(static_cast<std::size_t>(count + 2));
		if (this->read_surrogate_pairs_buf) tmp = this->read_surrogate_pairs_buf;
		tmp.resize(static_cast<std::size_t>((this->read_surrogate_pairs_buf) ? count + 1 : count));
		//read
		const auto read_num = this->s_buf_ptr->sgetn(&tmp[0], count);
		if (read_num <= 0) return L"";
		tmp.resize(static_cast<std::size_t>(read_num));
		//get surrogate pairs 2nd element. we don't have to think about combining character.
		if (is_utf16_surrogate_pairs_first_element(tmp.back())) {
			if (wtraits::eq_int_type(wtraits::eof(), this->sgetc())) {
				this->read_surrogate_pairs_buf = tmp.back();
				tmp.pop_back();
			}
			tmp += wtraits::to_char_type(this->sbumpc());
		}
		return tmp;
	}
protected:
	//this->eback() : 1st argument of this->setg()
	//this->gptr()  : 2nd argument of this->setg()
	//this->egptr() : 3rd argument of this->setg()

	// get a character from stream, but don't point past it
	//The public functions of std::streambuf call this function only if gptr() == nullptr or gptr() >= egptr().
	virtual int_type underflow() override {
		change_mode(mode::read);

		//check buffer
		if (this->read_buf.empty()) {
			//read
			const auto tmp = this->read_wstream_buf();
			if (tmp.empty())  return Traits::eof();
			this->mode = mode::read;
			//convert
			this->read_buf = this->wcvt.to_bytes(tmp);//move
			if (this->read_buf.empty()) return 0;
		}
		return Traits::to_int_type(read_buf.front());
	}
	virtual int_type uflow() override {
		const auto re = this->underflow();
		this->read_buf.erase(0, 1);
		if (this->read_buf.empty()) change_mode(mode::writeable);
		return re;
	}
	// put a character back to stream
	virtual int_type pbackfail(int_type c = Traits::eof()) override {
		change_mode(mode::read);
		if (c != Traits::eof()) {
			this->read_buf.insert(this->read_buf.begin(), c);
			this->mode = mode::read;
		}
		return c;
	}
	virtual std::streamsize xsgetn(char_type* s, std::streamsize count) override {
		change_mode(mode::read);
		if (count <= 0) return count;

		if (this->read_buf.empty()) {
			//read
			const auto tmp = this->read_wstream_buf(count);
			if (tmp.empty()) return 0;
			//convert
			this->read_buf = this->wcvt.to_bytes(tmp);//move
			if (this->read_buf.empty()) return 0;
		}
		else if (this->read_buf.size() < static_cast<std::size_t>(count)) {
			//read
			const auto tmp = this->read_wstream_buf(count - static_cast<std::streamsize>(read_buf.size()));
			//convert
			if (tmp.empty()) this->read_buf += this->wcvt.to_bytes(tmp);//copy back
		}
		const bool need_buf_erase = (static_cast<std::size_t>(count) < this->read_buf.size());
		const auto output_size = (!need_buf_erase) ? this->read_buf.size() : static_cast<std::size_t>(count);
		memcpy(s, this->read_buf.c_str(), output_size);
		if (need_buf_erase) {
			this->read_buf.erase(0, output_size);
		}
		else {
			this->read_buf.clear();
			change_mode(mode::writeable);
		}
		return 0;
	}
private:
	static std::size_t how_many_utf8_byte_from_last_is_broken(const std::string& s) {
		//                     byte per codepoint :    2     3     4     5     6
		//                         rest charactor :    1     2     3     4     5
		static constexpr std::uint8_t mask_list[] = { 0xE0, 0xF0, 0xF8, 0xFC, 0xFE };
		static constexpr std::uint8_t comp_list[] = { 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
		static_assert(sizeof(mask_list) == sizeof(comp_list), "error");

		if (s.empty()) return 0;
		const auto iterate_num = std::min(s.size(), sizeof(mask_list) / sizeof(std::uint8_t));
		std::size_t i = 0;
		for (auto rit = s.rbegin(); i < iterate_num; ++i, ++rit) {
			for (std::size_t j = i; j < sizeof(mask_list) / sizeof(std::uint8_t); ++j) {
				if (comp_list[j] == (static_cast<std::uint8_t>(*rit) & mask_list[j])) return i + 1;
			}
		}
		return 0;
	}
	void write_wstream_buf(bool rest_delete_flg = false){
		change_mode(mode::writeable);
		if (this->write_buf.empty()) return;
		const auto rest_size = how_many_utf8_byte_from_last_is_broken(this->write_buf);
		const auto buf_size = this->write_buf.size();
		if (buf_size == rest_size) return;
		const auto rest_front_pos = buf_size - rest_size - 1;
		auto tmp = std::move(this->write_buf);
		if (!rest_delete_flg) {
			this->write_buf = tmp.substr(rest_front_pos);
		}
		tmp.erase(rest_front_pos);
		const auto converted = this->wcvt.from_bytes(tmp);
		this->s_buf_ptr->sputn(converted.c_str(), static_cast<std::streamsize>(converted.size()));
	}
protected:
	//The sputc() and sputn() call this function in case of an overflow (pptr() == nullptr or pptr() >= epptr()).
	virtual int_type overflow(int_type _Meta = Traits::eof()) override {
		if (Traits::eq_int_type(Traits::eof(), _Meta))
			return (Traits::not_eof(_Meta));	// EOF, return success code

	}
	virtual int sync() override {
		return -1;
	}
	virtual pos_type seekoff(
		off_type /*off*/, std::ios_base::seekdir /*dir*/,
		std::ios_base::openmode /*which*/ = std::ios_base::in | std::ios_base::out
	) override {
		return pos_type(-1);	// always fail
	}
	virtual pos_type seekpos(
		pos_type pos,
		std::ios_base::openmode which = std::ios_base::in | std::ios_base::out
	) override {
		//return this->seekoff(off_type(pos), std::ios_base::beg, which);
		return pos_type(-1);	// always fail
	}
};
