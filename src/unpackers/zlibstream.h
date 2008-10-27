#ifndef ZLIBSTREAM_H
#define ZLIBSTREAM_H

#include <boost/iostreams/stream.hpp>
#include <boost/shared_ptr.hpp>
#include <zlib.h>
#include <iostream>

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
class zlib_buf {
	private:
		zlib_buf(zlib_buf &) : save_mask(NULL) { abort(); }
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
		zlib_buf(std::istream *st);
		~zlib_buf();

		std::streamsize read(char *s, std::streamsize n);

};


class zlib_source {
	protected:
		boost::shared_ptr<zlib_buf> bufp;
	public:
		zlib_source(std::istream *is) {
			bufp = boost::shared_ptr<zlib_buf>(new zlib_buf(is));
		}
		zlib_source(const zlib_source &zs) : bufp(zs.bufp) { }

		typedef char char_type;
		typedef boost::iostreams::source_tag category;


		std::streamsize read(char *s, std::streamsize n) {
			return bufp->read(s, n);
		}
};
#endif
