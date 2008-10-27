#ifndef BREEDPACK_H
#define BREEDPACK_H 1

#include <istream>
#include "unpacker.h"

class BreedUnpacker : public Unpacker {
	protected:
		std::istream *is;
		bool looks_valid;
		std::streampos zlibstart;
		std::map<std::string, std::string> metadata_cache;

		bool validate();
	public:
		BreedUnpacker(std::istream *is) {
			this->is = is;
			looks_valid = validate();
		}

		virtual bool recognizable() const {
			return looks_valid;
		}
		virtual const std::map<std::string, std::string> metadata() const {
			return metadata_cache;
		}
		virtual void unpack(
				const boost::function<
					void (const std::string &, std::istream &)
				> &callback
			) const;
};

#endif
