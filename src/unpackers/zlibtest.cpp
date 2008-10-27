#include <boost/iostreams/stream.hpp>
#include "zlibstream.h"
#include <iostream>
#include <fstream>

int main(int argc, char **argv) {
	try {
		if (argc != 2) {
			std::cerr << "Usage: "
				<< (argc ? argv[0] : "zlibtest")
				<< " filename"
				<< std::endl;
			return 1;
		}
		std::ifstream ifs(argv[1], std::ios_base::in | std::ios_base::binary);
		if (ifs.fail()) {
			std::cerr << "open failure" << std::endl;
			return 1;
		}
		{
			boost::iostreams::stream<zlib_source> zs(&ifs);
			char buf[4096];
			std::streamsize count = 0;
			std::cout << "zsource open\n";
			while (!zs.eof()) {
				zs.read(buf, sizeof(buf));
				std::streamsize n = zs.gcount();
				count += n;
			}
			std::cout << "read " << count << " bytes compressed\n";
		}
		std::cout << "zst destructed\n";
		std::streampos p = ifs.tellg();
		ifs.seekg(0, ifs.end);
		std::streampos eos = ifs.tellg();
		std::cout << "remained: " << eos - p << " bytes\n";
		return 0;
	} catch (std::exception &e) {
		std::cerr << "main: caught exception " << e.what() << std::endl;
		return 1;
	}
}
