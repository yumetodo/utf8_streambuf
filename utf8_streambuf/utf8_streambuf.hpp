#pragma once
#include <string>
#include <iostream>
#include <locale>
#include <codecvt>
class utf8_streambuf : public std::streambuf {
private:
	using Traits = traits_type;
	using wtraits = std::char_traits<wchar_t>;
	std::wstreambuf *is_ptr;
	std::wostream *os_ptr;
	std::string read_buf;//utf-8 buffer
	std::string write_buf;
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wcvt;
	enum class mode { unused, read, write } mode;
public:
	utf8_streambuf(std::wstreambuf *pStream)
		: is_ptr(pStream), os_ptr(), read_buf(), write_buf(1000, ' '), wcvt(), mode(mode::unused)
	{
		//this->setg(&read_buf[0], &read_buf[0], &read_buf[read_buf.length()]);//update read ptr
		this->setp(&write_buf[0], &write_buf[write_buf.length()]);//update write ptr
	}
protected:
	//this->eback() : 1st argument of this->setg()
	//this->gptr()  : 2nd argument of this->setg()
	//this->egptr() : 3rd argument of this->setg()

	// get a character from stream, but don't point past it
	//The public functions of std::streambuf call this function only if gptr() == nullptr or gptr() >= egptr().
	virtual int_type underflow() override {
		if (mode::write == this->mode) throw std::runtime_error("utf8_streambuf");

		//check buffer
		if (read_buf.empty()) {
			//read
			const auto tmp = this->read_wstream_buf();
			if (tmp.empty())  return Traits::eof();
			this->mode = mode::read;
			//convert
			read_buf = this->wcvt.to_bytes(tmp);//move
			if (read_buf.empty()) return 0;
		}
		return Traits::to_int_type(read_buf.front());
	}
	virtual int_type uflow() override {
		const auto re = this->underflow();
		read_buf.erase(0, 1);
		if (read_buf.empty()) this->mode = mode::unused;
		return re;
	}
	// put a character back to stream
	virtual int_type pbackfail(int_type c = Traits::eof()) override {
		if (mode::write == this->mode) throw std::runtime_error("utf8_streambuf");
		if (c != Traits::eof()) {
			read_buf.insert(read_buf.begin(), c);
			this->mode = mode::read;
		}
		return c;
	}
	virtual std::streamsize xsgetn(char_type* s, std::streamsize count) override {
		if (mode::write == this->mode) throw std::runtime_error("utf8_streambuf");
		if (count <= 0) return count;

		if (read_buf.empty()) {
			//read
			const auto tmp = this->read_wstream_buf(count);
			if (tmp.empty()) return 0;
			//convert
			read_buf = this->wcvt.to_bytes(tmp);//move
			if (read_buf.empty()) return 0;
		}
		else if (read_buf.size() < static_cast<std::size_t>(count)) {
			//read
			const auto tmp = this->read_wstream_buf(count - static_cast<std::streamsize>(read_buf.size()));
			//convert
			if (tmp.empty()) read_buf += this->wcvt.to_bytes(tmp);//copy back
		}
		const bool need_buf_erase = (static_cast<std::size_t>(count) < read_buf.size());
		const auto output_size = (!need_buf_erase) ? read_buf.size() : static_cast<std::size_t>(count);
		memcpy(s, read_buf.c_str(), output_size);
		if (need_buf_erase) {
			read_buf.erase(0, output_size);
			this->mode = mode::read;
		}
		else {
			read_buf.clear();
			this->mode = mode::unused;
		}
		return 0;
	}
private:
	static constexpr bool is_utf16_surrogate_pairs_first_element(wchar_t c) { return (2 == sizeof(wchar_t) && 0xD800 <= c && c <= 0xDBFF); }
	std::wstring read_wstream_buf(std::streamsize count = 1) {
		if (count < 0) throw std::out_of_range("utf8_streambuf");
		std::wstring tmp;
		//allocate
		tmp.reserve(static_cast<std::size_t>(count + 1));
		tmp.resize(static_cast<std::size_t>(count));
		//read
		const auto read_num = is_ptr->sgetn(&tmp[0], count);
		if (read_num <= 0) return L"";
		tmp.resize(static_cast<std::size_t>(read_num));
		//get surrogate pairs 2nd element. we don't have to think about combining character.
		if (is_utf16_surrogate_pairs_first_element(tmp.back())) {
			tmp += wtraits::to_char_type(this->sbumpc());
		}
		return tmp;
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
