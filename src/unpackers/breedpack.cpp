#include "breedpack.h"
#include "zlibstream.h"
#include "endianlove.h"
#include "openc2e.h"
#include "limit_filter.h"
#include <boost/iostreams/filtering_stream.hpp>

namespace io = boost::iostreams;

#define MAX_STRING 65536
#define MAX_FILES 65536
#define MAX_FILESIZE 100000000 // approx 100mb
static void checkedread(std::istream *is, char *buf, std::streamsize n) {
	is->read(buf, n);
	if (is->gcount() != n)
		throw creaturesException("Premature EOF");
}

static void skipstream(std::istream *is, std::streamsize n) {
	char buf[4096];

	while (n) {
		std::streamsize ct = n;
		if (ct > (std::streamsize)sizeof(buf))
			ct = sizeof(buf);
		checkedread(is, buf, ct);
		n -= ct;
	}
}

static uint32 readlong(std::istream *is) {
	uint32 l;
	checkedread(is, (char*)&l, sizeof l);
	l = swapEndianLong(l);
	return l;
}

static std::string readstr(std::istream *is) {
	uint32 len;
	std::vector<char> cbuf;

	len = readlong(is);
	if (len > MAX_STRING)
		throw creaturesException("String too large in unpacker metadata");
	cbuf.resize(len);

	checkedread(is, &cbuf[0], len);
	return std::string(&cbuf[0], len);
}

bool BreedUnpacker::validate() {
	try {
		is->seekg(-4, is->end);
		uint32 magic = readlong(is);
		if (magic != 0x66726264)
			return false;
		is->seekg(-8, is->end);
		uint32 offset = readlong(is);
		if (offset > is->tellg())
			return false;
		zlibstart = offset;

		is->seekg(zlibstart, is->beg);
		io::stream<zlib_source> zst(is);
		metadata_cache["title"] = readstr(&zst);
		metadata_cache["author"] = readstr(&zst);
		metadata_cache["description"] = readstr(&zst);

		return true;
	} catch (std::exception &e) {
		return false;
	}
}

void BreedUnpacker::unpack(
		const boost::function<
			void (const std::string &, std::istream &)
		> &callback
	) const
{
	is->seekg(zlibstart, is->beg);
	zlib_source zsrc(is);
	io::stream<zlib_source> zst(zsrc);

	// skip metadata
	readstr(&zst);
	readstr(&zst);
	readstr(&zst);

	uint32 filect = readlong(&zst);
	if (filect > MAX_FILES)
		throw creaturesException("too many files in pack");
	for (uint32 i = 0; i < filect; i++) {
		std::string name = readstr(&zst);
		uint32 filesz = readlong(&zst);
		if (filesz > MAX_FILESIZE)
			throw creaturesException("file too large in pack");
		io::stream<limit_source> st(&zst, filesz);
		callback(name, st);
	}	
}
