#include "breedpack.h"
#include <boost/typeof/std/utility.hpp>
#include <fstream>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/null.hpp>


namespace io = boost::iostreams;

static void cb(const std::string &filename, std::istream &is) {
	std::cout << "File " << filename << " size=";
	std::streamsize sz = io::copy(is, io::basic_null_sink<char>());
	std::cout << sz << std::endl;
}

int main(int argc, char **argv) {
	try {
		std::ifstream is(argv[1]);
		BreedUnpacker up(&is);
		if (!up.recognizable()) {
			std::cerr << "bad file\n";
		} else {
			std::cerr << "good file\n";
			std::map<std::string, std::string> meta = up.metadata();
			for(BOOST_AUTO(it, meta.begin()); it != meta.end(); it++) {
				std::cout << it->first << ": " << it->second << std::endl;
			}
			std::cout << "TOC:\n";
			up.unpack(cb);
		}
		return 0;
	} catch (std::exception &e) {
		std::cerr << e.what();
	}
}
