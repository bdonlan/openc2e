#ifndef UNPACKER_H
#define UNPACKER_H 1

#include <iostream>
#include <boost/function.hpp>

class Unpacker {
	protected:
		Unpacker() { }
	public:
		virtual ~Unpacker() { }

		virtual bool recognizable() = 0;
		virtual std::map<std::string, std::string> metadata() = 0;
		virtual void unpack(
				boost::function<
					void (const std::string &, std::istream &)
				> callback
			) = 0;
};

class UnpackerFactory {
	public:
		UnpackerFactory() { }
		virtual Unpacker *build(std::istream *s) = 0;
};

template <class T>
class UnpackerFactoryImpl : public UnpackerFactory {
	public:
		UnpackerFactoryImpl() { }
		Unpacker *build(std::istream *s) {
			Unpacker *u = new T(s);
			if (u->recognizable())
				return u;
			delete u;
			return NULL;
		}
};

#endif
