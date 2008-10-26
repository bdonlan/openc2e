#include "zlibstream.h"

zlib_source::zlib_source(std::istream *is) : save_mask(is) {
	int zret;
	// enable all exceptions
	is->exceptions(
			std::ios_base::iostate::badbit
			);
	// make sure we can seek
	is->seekg(0, std::ios_base::cur);

	zst.next_in = inbuf;
	zst.avail_in = 0;
	zst.zalloc = zst.zfree = zst.opaque = Z_NULL;

	refill();

	zret = inflateInit(&zst);
	if (zret != Z_OK)
		throw zlib_exception("Failed to initialize inflater");
	eos = 0;
}

zlib_source::~zlib_source() {
	rewind();
	inflateEnd(&zst);

}
void zlib_source::rewind() {
	if (is->fail()) // we're already aborting
		return;
	// return the unused portion of the zlib buffer to the stream
	is->seekg(-zst.avail_in, std::ios_base::cur);
}

void zlib_source::refill() {
	char *readbase = inbuf;
	if (zst.avail_in == sizeof(inbuf))
		return;
	if (zst.avail_in) {
		memmove((void *)inbuf, (void *)zst.next_in, zst.avail_in);
		readbase += zst.avail_in;
	}
	is->read(readbase, sizeof(inbuf) - zst.avail_in);
	zst.avail_in += is->gcount();
	if (!is->gcount())
		throw zlib_exception("Premature EOF");
}

std::streamsize zlib_source::read(char *s, std::streamsize n) {
	int zret;
	zst.next_out = s;
	zst.avail_out = n;
	if (eos)
		return -1;
	do {
		if (!zst.avail_in)
			refill();
		zret = inflate(&zst, Z_NO_FLUSH);
	} while (zst.avail_out && zret == Z_OK);
   switch (zret) {
	   case Z_STREAM_END:
		   eos = 1;
		   // fall through
	   case Z_OK:
		   return n - zst.avail_out;
	   case Z_NEED_DICT:
	   case Z_DATA_ERROR:
		   throw zlib_exception("Malformed zlib stream");
	   case Z_MEM_ERROR:
	   case Z_STREAM_ERROR:
	   case Z_BUF_ERROR:
		   assert(!"Internal zlib error");
		   throw zlib_exception("Internal zlib error");
	   default:
		   assert(!"Unknown zlib return");
		   throw zlib_exception("Unknown zlib return");
   }
}
