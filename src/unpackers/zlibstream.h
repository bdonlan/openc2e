#ifndef ZLIBSTREAM_H
#define ZLIBSTREAM_H

#include <boost/iostreams/stream.hpp>
#include <zlib.h>
#include <iostream>

namespace io = boost::iostreams;

class zlib_exception : public std::exception {
	protected:
		const char *whatbuf;
		char *alloc_what;
	public:
		zlib_exception(const char *what) throw() {
			this->whatbuf = what;
			this->alloc_what = NULL;
		}
		zlib_exception(const std::string &s) {
			this->alloc_what = strdup(s.c_str());
			if (!this->alloc_what)
				throw std::bad_alloc();
			this->whatbuf = alloc_what;
		}
		const char *what() const throw() {
			return this->whatbuf;
		}
		~zlib_exception() throw () {
			free(alloc_what);
		}
};

/* This is a source because the core iostream API doesn't support rewinding
 * unused data off a filter - thus the trailer data after the zlib stream
 * would be lost
 */
class zlib_source {
	private:
		zlib_source(zlib_source &) : save_mask(NULL) { abort(); }
	protected:
		class saveexcept {
			protected:
				std::istream *is;
				std::ios::iostate except_mask;
			public:
				saveexcept(std::istream *is) {
					this->is = is;
					this->except_mask = is->exceptions();
				}
				~saveexcept() {
					is->exceptions(this->except_mask);
				}
		};
		std::istream *is;
		z_stream zst;
		char inbuf[16384];
		int eos;
		std::streampos initpos;
		saveexcept save_mask;

		void refill();
		void rewind();
	public:
		typedef char char_type;
		typedef io::source_tag category;

		zlib_source(std::istream *st);
		~zlib_source();

		std::streamsize read(char *s, std::streamsize n);

};
#endif
