/*
 *  quadtree_test.cpp
 *  openc2e
 *
 *  Copyright (c) 2006-2008 Bryan Donlan. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 */
#include "staticquadtree.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <cstdio>
#include <boost/lambda/if.hpp>

using namespace boost::lambda;

#if 0
struct rcmp
{
	bool operator()(const QuadRegion &r1, const QuadRegion &r2) 
	{
#define CMP(v) \
		do { if (r1.v != r2.v) return r1.v < r2.v; } while (0)
		CMP(left); CMP(right); CMP(top); CMP(bottom);
#undef CMP
	}
};
#endif

const QuadRegion *exhaustivesearch(const std::vector<std::pair<QuadRegion, QuadRegion> > &l, int x, int y, QuadRegion *t) {
	for (BOOST_AUTO(i, l.begin()); i != l.end(); i++) {
		if (i->first.contains(x, y) && i->second == *t)
			return &i->second;
	}
	return NULL;
}

bool quadsearch(const StaticQuadTree<QuadRegion> &qtree, int x, int y, QuadRegion *t) {
	bool found;
	qtree.lookup(x, y, if_then_else_return(
				_1 == *t,
				( var(found) = true, true ),
				false));
	return found;
}

struct callback {
	typedef QuadRegion Region;
	Region goal;
	bool &found;
	bool operator()(const Region &r) {
		if (r == goal)
			found = true;
		return found;
	}

	callback(Region g, bool &f) : goal(g), found(f) { }
} ;
int main() {
	using namespace boost::lambda;
	typedef QuadRegion Region;
	unsigned int seed = time(NULL);
	seed = 1225213535;
	srand(seed);
	std::cerr << "Seed: " << seed << std::endl;
	std::vector<std::pair<Region, Region> > rlist;
#define ROOM(x1, y1, x2, y2) do { \
	Region r(x1, x2, y1, y2); \
	rlist.push_back(std::pair<Region, Region>(r, r)); \
} while(0)

	// c2 room list
ROOM(1643, 340, 2146, 632);
ROOM(1553, 340, 1643, 632);
ROOM(1211, 340, 1553, 639);
ROOM(975, 340, 1211, 682);
ROOM(2146, 340, 2480, 632);
ROOM(2480, 340, 3077, 671);
ROOM(3077, 340, 3768, 671);
ROOM(3768, 340, 4183, 632);
ROOM(4183, 340, 4300, 595);
ROOM(4300, 340, 4894, 632);
ROOM(4894, 340, 5453, 632);
ROOM(5453, 340, 5735, 632);
ROOM(5735, 462, 5870, 875);
ROOM(5870, 462, 6565, 669);
ROOM(5870, 669, 6355, 914);
ROOM(5453, 632, 5735, 941);
ROOM(4894, 632, 5453, 941);
ROOM(4680, 632, 4894, 941);
ROOM(4358, 810, 4680, 888);
ROOM(4358, 888, 4680, 941);
ROOM(4300, 632, 4358, 941);
ROOM(4183, 595, 4300, 840);
ROOM(3973, 632, 4183, 840);
ROOM(4183, 840, 4300, 1175);
ROOM(3917, 840, 4183, 1200);
ROOM(3768, 632, 3973, 840);
ROOM(4300, 941, 4894, 1200);
ROOM(4894, 941, 5453, 1200);
ROOM(5453, 941, 5735, 1200);
ROOM(5735, 875, 5870, 1175);
ROOM(5870, 914, 6060, 1175);
ROOM(6355, 669, 6565, 914);
ROOM(3917, 1200, 4543, 1544);
ROOM(4075, 1739, 4349, 1858);
ROOM(4349, 1739, 4543, 1858);
ROOM(3917, 1544, 4349, 1739);
ROOM(4349, 1544, 4504, 1739);
ROOM(4504, 1544, 4543, 1739);
ROOM(4543, 1544, 4932, 1858);
ROOM(4932, 1615, 5365, 1858);
ROOM(4543, 1200, 4932, 1400);
ROOM(4932, 1200, 5365, 1615);
ROOM(5365, 1615, 5900, 1858);
ROOM(4800, 2295, 5700, 2399);
ROOM(4800, 2077, 5700, 2295);
ROOM(4543, 1858, 5130, 2077);
ROOM(5130, 1858, 5700, 2077);
ROOM(5700, 1858, 5900, 2255);
ROOM(5700, 2255, 5900, 2399);
ROOM(4543, 2077, 4800, 2399);
ROOM(4075, 1858, 4543, 2399);
ROOM(3434, 1544, 3917, 1747);
ROOM(3917, 1739, 4075, 2399);
ROOM(3828, 1747, 3917, 2399);
ROOM(3060, 1747, 3828, 1996);
ROOM(3645, 1996, 3828, 2399);
ROOM(3515, 1996, 3645, 2261);
ROOM(3515, 2261, 3645, 2399);
ROOM(3060, 1996, 3515, 2399);
ROOM(2451, 1544, 3434, 1747);
ROOM(3006, 1285, 3917, 1544);
ROOM(2904, 840, 3368, 1285);
ROOM(3768, 840, 3917, 1285);
ROOM(2904, 671, 3768, 840);
ROOM(2480, 671, 2904, 990);
ROOM(2593, 1285, 3006, 1544);
ROOM(2593, 990, 2904, 1285);
ROOM(2320, 1322, 2593, 1453);
ROOM(2320, 990, 2593, 1322);
ROOM(2146, 632, 2480, 990);
ROOM(1943, 632, 2146, 806);
ROOM(1643, 806, 2146, 990);
ROOM(1643, 632, 1943, 806);
ROOM(1553, 632, 1643, 806);
ROOM(1553, 806, 1643, 990);
ROOM(1670, 1322, 2320, 1506);
ROOM(2068, 1506, 2247, 1565);
ROOM(1670, 1506, 2068, 1565);
ROOM(1473, 990, 2320, 1322);
ROOM(1546, 1322, 1670, 1496);
ROOM(1546, 1496, 1670, 2149);
ROOM(2068, 1565, 2288, 1860);
ROOM(2247, 1506, 2451, 1565);
ROOM(2320, 1453, 2451, 1506);
ROOM(2451, 1453, 2593, 1544);
ROOM(1800, 1600, 2068, 1860);
ROOM(1670, 1565, 2068, 1600);
ROOM(1670, 1600, 1800, 1956);
ROOM(1800, 1860, 2137, 1956);
ROOM(2206, 1956, 2308, 2149);
ROOM(2137, 1860, 2376, 1956);
ROOM(2376, 1860, 2451, 1956);
ROOM(2308, 1956, 2451, 2180);
ROOM(1670, 1956, 2206, 2180);
ROOM(2206, 2149, 2308, 2320);
ROOM(2206, 2320, 2800, 2399);
ROOM(2308, 2180, 2800, 2320);
ROOM(2550, 1747, 2915, 2180);
ROOM(2451, 2030, 2550, 2180);
ROOM(1450, 2149, 1670, 2180);
ROOM(1450, 2180, 2206, 2399);
ROOM(1058, 1980, 1450, 2399);
ROOM(1473, 1322, 1546, 1841);
ROOM(1450, 1841, 1546, 2149);
ROOM(802, 2164, 1058, 2399);
ROOM(0, 1841, 802, 2399);
ROOM(802, 1841, 1058, 2164);
ROOM(1058, 1841, 1450, 1980);
ROOM(1342, 1140, 1473, 1300);
ROOM(853, 1140, 1342, 1352);
ROOM(669, 1352, 1091, 1472);
ROOM(669, 1140, 853, 1352);
ROOM(1342, 1300, 1473, 1596);
ROOM(1091, 1352, 1342, 1596);
ROOM(1091, 1596, 1473, 1841);
ROOM(573, 1472, 1091, 1841);
ROOM(0, 1540, 573, 1841);
ROOM(0, 1503, 164, 1540);
ROOM(58, 1035, 164, 1503);
ROOM(164, 1240, 399, 1540);
ROOM(399, 1472, 573, 1540);
ROOM(399, 1220, 669, 1472);
ROOM(5900, 1175, 6060, 1470);
ROOM(6060, 914, 6391, 1470);
ROOM(58, 720, 164, 1035);
ROOM(164, 813, 300, 1096);
ROOM(164, 1096, 399, 1240);
ROOM(164, 720, 263, 813);
ROOM(0, 340, 300, 720);
ROOM(410, 340, 675, 630);
ROOM(675, 340, 975, 630);
ROOM(300, 682, 975, 720);
ROOM(263, 720, 300, 813);
ROOM(300, 340, 410, 682);
ROOM(669, 897, 840, 1140);
ROOM(500, 897, 669, 1096);
ROOM(300, 720, 840, 897);
ROOM(840, 720, 975, 990);
ROOM(5735, 340, 6192, 462);
ROOM(6192, 340, 6565, 462);
ROOM(6565, 340, 6880, 693);
ROOM(6880, 340, 7094, 693);
ROOM(7094, 340, 7380, 693);
ROOM(7530, 340, 8143, 693);
ROOM(8143, 340, 8352, 755);
ROOM(7306, 693, 7670, 906);
ROOM(7171, 826, 7306, 906);
ROOM(7171, 693, 7306, 826);
ROOM(6565, 693, 7171, 826);
ROOM(7087, 826, 7171, 906);
ROOM(7087, 906, 7350, 1175);
ROOM(6850, 826, 7087, 1175);
ROOM(6565, 826, 6850, 1175);
ROOM(6391, 914, 6565, 1175);
ROOM(6391, 1175, 6722, 1470);
ROOM(6722, 1175, 7220, 1470);
ROOM(7220, 1175, 7720, 1326);
ROOM(7350, 906, 7670, 1175);
ROOM(5900, 1470, 6113, 1664);
ROOM(6113, 1470, 6930, 1798);
ROOM(5900, 1664, 6113, 1897);
ROOM(6113, 1798, 6930, 1897);
ROOM(5900, 1897, 6430, 2399);
ROOM(6600, 1897, 6930, 2399);
ROOM(6930, 2234, 7289, 2399);
ROOM(7289, 2234, 7736, 2399);
ROOM(7736, 2234, 8040, 2399);
ROOM(8040, 1845, 8240, 2399);
ROOM(6930, 1897, 7289, 2234);
ROOM(7289, 1897, 7736, 2234);
ROOM(7736, 1897, 8040, 2234);
ROOM(6930, 1470, 7289, 1897);
ROOM(7289, 1470, 7736, 1897);
ROOM(7736, 1470, 8040, 1897);
ROOM(8040, 1470, 8240, 1845);
ROOM(8240, 1503, 8352, 1845);
ROOM(7934, 1304, 8240, 1470);
ROOM(7220, 1326, 7934, 1470);
ROOM(7670, 969, 7934, 1175);
ROOM(7670, 693, 7934, 969);
ROOM(0, 1182, 58, 1503);
ROOM(0, 720, 58, 1182);
ROOM(7934, 1166, 8240, 1304);
ROOM(8240, 1182, 8352, 1503);
ROOM(8240, 755, 8352, 1182);
ROOM(8143, 855, 8240, 1166);
ROOM(8143, 755, 8240, 855);
ROOM(7934, 693, 8143, 969);
ROOM(7934, 969, 8143, 1166);
ROOM(840, 990, 1473, 1140);
ROOM(975, 682, 1211, 782);
ROOM(1211, 639, 1553, 819);
ROOM(2915, 1747, 3060, 2180);
ROOM(1553, 150, 2146, 340);
ROOM(0, 150, 600, 340);
ROOM(2146, 150, 2700, 340);
ROOM(700, 50, 900, 150);
ROOM(900, 0, 1400, 150);
ROOM(6000, 150, 6300, 340);
ROOM(7200, 150, 7500, 340);
ROOM(975, 782, 1211, 990);
ROOM(1334, 819, 1553, 990);
ROOM(1211, 819, 1334, 990);
ROOM(300, 897, 500, 1096);
ROOM(399, 1096, 669, 1220);
ROOM(5365, 1200, 5735, 1615);
ROOM(2800, 2180, 3060, 2399);
ROOM(5735, 1300, 5900, 1615);
ROOM(5735, 1175, 5900, 1300);
ROOM(4183, 1175, 4300, 1200);
ROOM(4543, 1400, 4700, 1544);
ROOM(4700, 1400, 4932, 1544);
ROOM(6430, 2080, 6600, 2399);
ROOM(8240, 1845, 8352, 2399);
ROOM(6430, 1897, 6600, 2080);
ROOM(410, 630, 975, 682);
ROOM(600, 150, 900, 340);
ROOM(900, 150, 1211, 340);
ROOM(1211, 150, 1553, 340);
ROOM(4358, 632, 4680, 810);
ROOM(2700, 150, 3300, 340);
ROOM(3300, 150, 3600, 340);
ROOM(3600, 150, 3900, 340);
ROOM(3900, 150, 4200, 340);
ROOM(4200, 150, 4800, 340);
ROOM(4800, 150, 5100, 340);
ROOM(5100, 150, 5700, 340);
ROOM(7720, 1175, 7934, 1326);
ROOM(5700, 150, 6000, 340);
ROOM(6300, 150, 6600, 340);
ROOM(6600, 150, 6900, 340);
ROOM(6900, 150, 7200, 340);
ROOM(7500, 150, 7800, 340);
ROOM(7380, 340, 7530, 693);
ROOM(7800, 150, 8100, 340);
ROOM(8100, 150, 8352, 340);
ROOM(0, 0, 700, 150);
ROOM(700, 0, 900, 50);
ROOM(1400, 0, 2100, 150);
ROOM(2100, 0, 2800, 150);
ROOM(2800, 0, 3500, 150);
ROOM(3500, 0, 4200, 150);
ROOM(4200, 0, 4900, 150);
ROOM(4900, 0, 5600, 150);
ROOM(5600, 0, 6300, 150);
ROOM(6300, 0, 7000, 150);
ROOM(7000, 0, 7700, 150);
ROOM(7700, 0, 8352, 150);
ROOM(2451, 1747, 2550, 2030);
ROOM(2288, 1565, 2451, 1860);
ROOM(3368, 840, 3768, 1285);

	StaticQuadTree<Region> qtree(rlist);
	int ok = 0;
	int nok = 0;
	int exok = 0;

	for (BOOST_AUTO(i, rlist.begin()); i != rlist.end(); i++) {
		for (int j = 0; j < 500; j++) {
			int x = i->second.left + (rand() % i->second.width());
			int y = i->second.top + (rand() % i->second.height());

			if (exhaustivesearch(rlist, x, y, &i->first))
				exok++;
//			callback cbs(i->second, found);
			if (quadsearch(qtree, x, y, &i->first))
				ok++;
			else
				nok++;
//			qtree.lookup<callback>(x, y, cbs);
//			qtree.lookup(x, y, cbs);
//			qtree.lookup(x, y, if_then_else_return(
//						_1 == i->second,
//						( var(found) = true, true ),
//						false));
#if 0
			if (found) {
				ok++;
				lok++;
				goto doneone;
			}
#endif
#if 0
//			std::vector<Region> rets = qtree.lookup(x, y);
			for (BOOST_AUTO(r, rets.begin()); r != rets.end(); r++) {
				if (*r == i->second) {
					ok++;
					lok++;
					goto doneone;
				}
			}
#endif
#if 0
			std::cerr << "Can't find point (" << x << ", " << y << ") in room " << i->second.to_s() << std::endl;
			qtree.lookup(x, y);
			if (!i->second.contains(x, y)) {
				assert(i->second.contains(x,y));
			}
			nok++;
doneone:;
#endif
		}
//		std::cout << lok;
	}
	std::cout << std::endl;

	std::cout << "ok " << ok << " nok " << nok << " exok " << exok << std::endl;
	return 0;

}
