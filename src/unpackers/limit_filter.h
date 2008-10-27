#include <iosfwd>                          // streamsize
#include <boost/iostreams/categories.hpp>  // tags
#include <boost/iostreams/operations.hpp>  // read

class limit_buf {
	protected:
		std::istream *is;
		std::streamsize remain;
	public:
		limit_buf(std::istream *is_, std::streamsize s) : is(is_), remain(s) { }

		std::streamsize read(char *s, std::streamsize n)
		{
			if (!remain)
				return -1;
			if (n > remain)
				n = remain;
			is->read(s, n);
			std::streamsize ret = is->gcount();
			if (ret > 0) {
				remain -= ret;
				return ret;
			}
			return is->eof() ? -1 : 0;
		}
};

class limit_source {
	protected:
		boost::shared_ptr<limit_buf> bufp;
	public:
		limit_source(std::istream *is, std::streamsize n) {
			bufp = boost::shared_ptr<limit_buf>(new limit_buf(is, n));
		}
		limit_source(const limit_source &ls) : bufp(ls.bufp) { }

		typedef char char_type;
		typedef boost::iostreams::source_tag category;

		std::streamsize read(char *buf, std::streamsize n) {
			return bufp->read(buf, n);
		}
};
